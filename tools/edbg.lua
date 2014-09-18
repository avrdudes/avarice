-- Written by Joerg Wunsch
--
-- Wireshark LUA dissector for EDBG USB frames
--
-- To use, run wireshark with the option "-X lua_script:edbg.lua"

-- As this code uses LUA bindings for Wireshark, redistributions of this code are
-- required to be licensed under the GNU Public License, version 2 (or higher).
-- http://www.wireshark.org/faq.html#q1.1
--
-- See
-- http://www.gnu.org/licenses/gpl-2.0.html
-- for further information about the licensing conditions.

-- $Id$

-- do

   -- Wireshark dissector for EDBG USB frames

   local edbg_proto = Proto ("EDBG", "Atmel EDBG protocol")

   -- Set this to "true" to dissect boring de-facto constant fields in
   -- the AVR protocols
   local verbose = false

   local cmd_table = {
      -- (some) official CMSIS-DAP commands
      [0x00] = "Info",
      [0x01] = "LED",
      [0x02] = "Connect",
      [0x03] = "Disconnect",
      [0x04] = "Transfer configure",
      [0x0a] = "Reset target",
      [0x11] = "SWD/JTAG clock",
      [0x13] = "SWD configure",
      -- (some) Atmel vendor commands
      [0x80] = "AVR command",
      [0x81] = "AVR response",
      [0x82] = "AVR event",
   }

   local dir_table = {
      [0] = "Request",
      [1] = "Response"
   }

   local info_id_table = {
      [0x01] = "Vendor ID",
      [0x02] = "Product ID",
      [0x03] = "Serial number",
      [0x04] = "CMSIS-DAP firmware version",
      [0x05] = "Target device vendor",
      [0x06] = "Target device name",
      [0xf0] = "Capabilities",
      [0xfe] = "Maximum packet count",
      [0xff] = "Maximum packet length"
   }

   local led_id_table = {
      [0x00] = "Connect LED",
      [0x01] = "Running LED"
   }

   local led_status_table = {
      [0x00] = "Off",
      [0x01] = "On"
   }

   local dap_port_table = {
      [0x00] = "Default mode",
      [0x01] = "SWD mode",
      [0x02] = "JTAG mode"
   }

   local dap_port_status_table = {
      [0x00] = "Port init failed",
      [0x01] = "SWD init OK",
      [0x02] = "JTAG init OK"
   }

   local dap_rsp_status_table = {
      [0x00] = "OK",
      [0xff] = "Error"
   }

   local avr_subproto_table = {
      -- matches AVRDUDE and AVaRICE naming ("scope"), rather than Atmel naming
      [0x00] = "Info",
      [0x01] = "General",
      [0x11] = "AVR ISP",
      [0x12] = "AVR"
   }

   local avr_cmd_table = {
      -- again, follows AVRDUDE and AVaRICE naming
      -- these are used both, in general and AVR scope
      [0x01] = "Set parameter",
      [0x02] = "Get parameter",
      [0x10] = "Sign-on",
      [0x11] = "Sign-off",
      -- these are AVR-scope only
      [0x12] = "Device ID",
      [0x13] = "Start debugging",
      [0x14] = "Stop debugging",
      [0x15] = "Enter programming mode",
      [0x16] = "Leave programming mode",
      [0x17] = "MonCon disable",
      [0x20] = "Erase memory",
      [0x21] = "Read memory",
      [0x23] = "Write memory",
      [0x30] = "Reset",
      [0x31] = "Stop",
      [0x32] = "Go",
      [0x33] = "Run to",
      [0x34] = "Step",
      [0x35] = "Read PC",
      [0x36] = "Write PC",
      [0x40] = "Set BP",
      [0x41] = "Clear BP",
      [0x42] = "Set BP Xmega",
      [0x43] = "Set soft BP",
      [0x44] = "Clear soft BP",
      [0x45] = "Cleanup"
   }

   local avr_rsp_table = {
      [0x80] = "OK",
      [0x81] = "Info",
      [0x83] = "PC",
      [0x84] = "Data",
      [0xa0] = "Failed"
   }

   local avr_rsp_failure_table = {
      [0x10] = "Debugwire",
      [0x1b] = "PDI",
      [0x20] = "No answer",
      [0x22] = "No target power",
      [0x32] = "Wrong mode",
      [0x34] = "Unsupported memory type",
      [0x35] = "Wrong length",
      [0x91] = "Not understood"
   }

   local avr_rsp_status_table = {
      [0x00] = "OK"
      -- nothing else is explained
   }

   local avr_evt_table = {
      [0x10] = "Power",
      [0x11] = "Sleep/wakeup",
      [0x12] = "Reset",
      [0x40] = "Break",
      [0x41] = "IDR"
   }

   local avr_sleep_table = {
      [0x00] = "awake",
      [0x01] = "asleep"
   }

   local avr_power_table = {
      [0x00] = "off",
      [0x01] = "on"
   }

   local avr_reset_table = {
      [0x00] = "released",
      [0x01] = "applied"
   }

   local avr_break_cause_table = {
      [0x00] = "Unspecified",
      [0x01] = "Program breakpoint",
      -- guessed:
      [0x02] = "Data break PDSB",
      [0x03] = "Data break PDMSB"
   }

   local avr_parms_gen_sect0_table = {
      [0x00] = "Hardware version",
      [0x01] = "Firmware major version",
      [0x02] = "Firmware minor version",
      [0x03] = "Firmware release"
   }

   local avr_parms_gen_sect1_table = {
      [0x00] = "Vtarget"
   }

   local avr_parms_gen_sect2_table = {
      [0x00] = "Device description"
   }

   local avr_arch_table = {
      [1] = "Tiny",
      [2] = "Mega",
      [3] = "Xmega"
   }

   local avr_session_table = {
      [1] = "Programming",
      [2] = "Debugging"
   }

   local avr_parms_avr_sect0_table = {
      [0x00] = "Architecture",
      [0x01] = "Session purpose"
   }

   local avr_connection_table = {
      [1] = "ISP",
      [4] = "JTAG",
      [5] = "debugWIRE",
      [6] = "PDI"
   }

   local avr_parms_avr_sect1_table = {
      [0x00] = "Connection",
      [0x01] = "JTAG chain",
      [0x20] = "JTAG Programming clock, megaAVR",
      [0x21] = "JTAG Debugging clock, megaAVR",
      [0x30] = "JTAG Clock, Xmega",
      [0x31] = "PDI Clock, Xmega"
   }

   local avr_parms_avr_sect3_table = {
      [0x00] = "Timers running"
   }

   local avr_memory_types_table = {
      [0x20] = "SRAM",
      [0x22] = "EEPROM",
      [0xa0] = "Flash, LPM/SPM",
      [0xb0] = "Flash, paged",
      [0xb1] = "EEPROM, paged",
      [0xb2] = "Fuse bits",
      [0xb3] = "Lock bits",
      [0xb4] = "Signature",
      [0xb5] = "OSCCAL",
      [0xb8] = "Registers",
      [0xc0] = "Application flash",
      [0xc1] = "Boot flash",
      [0xc5] = "User signature",
      [0xc6] = "Production signature"
   }

   -- 0: OUT, 1: IN
   local dir = Field.new("usb.endpoint_number.direction")

   -- global edbg fields
   local f_cmd = ProtoField.uint8("edbg.cmd", "Command",
                                  base.HEX, cmd_table)
   local f_dir = ProtoField.uint8("edbg.dir", "Direction",
                                  base.DEC, dir_table)

   local f_status = ProtoField.uint8("edbg.status", "DAP status",
                                     base.HEX, dap_rsp_status_table)

   local f_dap_info_id = ProtoField.uint8("edbg.info.id", "DAP info ID",
                                          base.HEX, info_id_table)
   local f_dap_info_string = ProtoField.string("edbg.info.string", "Info string")
   local f_dap_info_byte = ProtoField.uint8("edbg.info.byte", "Info BYTE",
                                            base.DEC)
   local f_dap_info_short = ProtoField.uint16("edbg.info.short", "Info SHORT",
                                              base.DEC)

   local f_dap_led_id = ProtoField.uint8("edbg.led.id", "DAP LED ID",
                                         base.HEX, led_id_table)
   local f_dap_led_status = ProtoField.uint8("edbg.led.status", "DAP LED status",
                                             base.HEX, led_status_table)

   local f_dap_port = ProtoField.uint8("edbg.port", "DAP port",
                                       base.HEX, dap_port_table)
   local f_dap_port_status = ProtoField.uint8("edbg.port_status", "DAP port status",
                                              base.HEX, dap_port_status_table)

   local f_dap_swj_clock = ProtoField.uint32("edbg.swj.clock", "SWJ clock (Hz)",
                                             base.DEC)

   local f_xfr_idle_cycles = ProtoField.uint8("edbg.xfr.idle_cycles",
                                              "Number of extra idle cycles after each transfer",
                                              base.DEC)
   local f_xfr_wait_retry = ProtoField.uint16("edbg.xfr.wait_retry",
                                              "Number of transfer retries after WAIT response",
                                              base.DEC)
   local f_xfr_match_retry = ProtoField.uint16("edbg.xfr.match_retry",
                                               "Number of retries on reads with Value Match in DAP_Transfer",
                                               base.DEC)

   local f_avr_fragment_info = ProtoField.uint8("edbg.avr.fragment_info",
                                                "Fragment info",
                                                base.HEX)
   local f_avr_size = ProtoField.uint16("edbg.avr.size", "Wrapped size",
                                        base.DEC)
   local f_avr_fragment_code = ProtoField.uint8("edbg.avr.fragment_code",
                                                "Fragment code",
                                                base.HEX)
   local f_avr_packet = ProtoField.bytes("edbg.avr.packet", "AVR packet")
   local f_avr_sof = ProtoField.uint8("edbg.avr.sof", "AVR SOF", base.HEX)
   local f_avr_proto_version = ProtoField.uint8("edbg.avr.protocol_version",
                                                "Protocol version", base.HEX)
   local f_avr_seq_id = ProtoField.uint16("edbg.avr.seq_id", "Sequence ID",
                                          base.DEC)
   local f_avr_subproto_id = ProtoField.uint8("edbg.avr.subproto_id",
                                              "Subprotocol ID",
                                              base.HEX, avr_subproto_table)
   local f_avr_cmd = ProtoField.uint8("edbg.avr.cmd", "Subprotocol command",
                                      base.HEX, avr_cmd_table)
   local f_avr_rsp_code = ProtoField.uint8("edbg.avr.rsp_code", "Response code",
                                           base.HEX, avr_rsp_table)
   local f_avr_rsp_contents = ProtoField.bytes("edbg.avr.rsp_contents", "Response contents")
   local f_avr_pc = ProtoField.uint32("edbg.avr.pc", "Program counter", base.HEX)
   local f_avr_rsp_failure_code = ProtoField.uint8("edbg.avr.pc", "Failure code",
                                                   base.HEX, avr_rsp_failure_table)
   local f_avr_rsp_status = ProtoField.uint8("edbg.avr.rsp_status", "Response status",
                                             base.HEX, avr_rsp_status_table)

   local f_avr_evt_code = ProtoField.uint8("edbg.avr.evt_code", "Event code",
                                           base.HEX, avr_evt_table)
   local f_avr_evt_power_state = ProtoField.uint8("edbg.avr.power_state",
                                                  "Power state",
                                                  base.HEX, avr_power_table)
   local f_avr_evt_sleep_state = ProtoField.uint8("edbg.avr.sleep_state",
                                                  "Sleep state",
                                                  base.HEX, avr_sleep_table)
   local f_avr_evt_reset_state = ProtoField.uint8("edbg.avr.reset_state",
                                                  "State of external reset",
                                                  base.HEX, avr_reset_table)
   local f_avr_break_cause = ProtoField.uint8("edbg.avr.break_cause",
                                              "Break cause",
                                              base.HEX, avr_break_cause_table)
   local f_avr_idr = ProtoField.uint8("edbg.avr.idr_message",
                                      "IDR message from MCU",
                                      base.HEX)
   local f_avr_ext_info = ProtoField.uint16("edbg.avr.ext_info",
                                            "Extended info",
                                            base.HEX)
   local f_avr_parms_sect = ProtoField.uint8("edbg.avr.parms.section",
                                             "Parameter section (context)",
                                             base.HEX)
   local f_avr_parms_gen_sect0 = ProtoField.uint8("edbg.avr.parms.gen_sect0",
                                                  "General parameters, section 0",
                                                  base.HEX, avr_parms_gen_sect0_table)
   local f_avr_parms_gen_sect1 = ProtoField.uint8("edbg.avr.parms.gen_sect1",
                                                  "General parameters, section 1",
                                                  base.HEX, avr_parms_gen_sect1_table)
   local f_avr_parms_gen_sect2 = ProtoField.uint8("edbg.avr.parms.gen_sect2",
                                                  "General parameters, section 2",
                                                  base.HEX, avr_parms_gen_sect2_table)
   local f_avr_parms_avr_sect0 = ProtoField.uint8("edbg.avr.parms.avr_sect0",
                                                  "AVR parameters, section 0",
                                                  base.HEX, avr_parms_avr_sect0_table)
   local f_avr_parms_avr_sect1 = ProtoField.uint8("edbg.avr.parms.avr_sect1",
                                                  "AVR parameters, section 1",
                                                  base.HEX, avr_parms_avr_sect1_table)
   local f_avr_parms_avr_sect3 = ProtoField.uint8("edbg.avr.parms.avr_sect3",
                                                  "AVR parameters, section 3",
                                                  base.HEX, avr_parms_avr_sect3_table)
   local f_avr_parms_avr_arch = ProtoField.uint8("edbg.avr.parms.avr.arch",
                                                 "AVR Architecture",
                                                 base.DEC, avr_arch_table)
   local f_avr_parms_avr_session = ProtoField.uint8("edbg.avr.parms.avr.sess",
                                                    "Session purpose",
                                                    base.DEC, avr_session_table)
   local f_avr_parms_avr_connection = ProtoField.uint8("edbg.avr.parms.avr.conn",
                                                       "Connection type",
                                                       base.DEC, avr_connection_table)
   local f_avr_parms_nbytes = ProtoField.uint8("edbg.avr.parms.nbytes",
                                               "Number of parameter bytes to set/get",
                                               base.DEC)
   local f_avr_parms_bytes = ProtoField.bytes("edbg.avr.parms.bytes",
                                              "Parameter bytes")
   local f_avr_parms_avr_clock = ProtoField.uint16("edbg.avr.parms.avr.clock",
                                                   "Clock value (kHz)",
                                                   base.DEC)
   local f_avr_mem_type = ProtoField.uint8("edbg.avr.memory.type",
                                           "Memory type",
                                           base.HEX, avr_memory_types_table)
   local f_avr_mem_addr = ProtoField.uint32("edbg.avr.memory.address",
                                            "Memory start address",
                                            base.HEX)
   local f_avr_mem_size = ProtoField.uint32("edbg.avr.memory.size",
                                            "Memory size",
                                            base.HEX)
   local f_avr_mem_bytes = ProtoField.bytes("edbg.avr.memory.bytes",
                                            "Memory contents")

   -- helper variables
   local last_req = "none"

   -- local dissectors
   function dissect_dap_info_req(buffer, len, pinfo, tree)
      tree:add(f_dap_info_id, buffer(0, 1))
      if buffer(0, 1):uint() < 0xf0 then
         last_req = "string"
      else
         last_req = "uint"
      end
   end

   function dissect_dap_info_rsp(buffer, len, pinfo, tree)
      local rsplen = buffer(0, 1):uint()
      if rsplen <= len - 1 then
         if last_req == "string" then
            -- string is preceded by length, and followed by \x00
            tree:add(f_dap_info_string, buffer(1, rsplen))
         elseif last_req == "uint" then
            -- either BYTE or SHORT
            if rsplen == 1 then
               tree:add(f_dap_info_byte, buffer(1, 1))
            else
               tree:add_le(f_dap_info_short, buffer(1, 2))
            end
         end
      end
      last_req = "none"
   end

   function dissect_avr_cmd_req(buffer, len, pinfo, tree)
      if len >= 9 then
         if verbose then tree:add(f_avr_fragment_info, buffer(0, 1)) end
         tree:add(f_avr_size, buffer(1, 2))
         local wrappedsize = buffer(1, 2):uint()
         local subtree = tree:add(f_avr_packet, buffer(3, wrappedsize))
         if verbose then
            subtree:add(f_avr_sof, buffer(3, 1))
            subtree:add(f_avr_proto_version, buffer(4, 1))
         end
         subtree:add_le(f_avr_seq_id, buffer(5, 2))
         subtree:add(f_avr_subproto_id, buffer(7, 1))
         subtree = subtree:add(f_avr_cmd, buffer(8, 1))
         -- byte 9 is "version"; left out for brevity (always 0
         local cmdval = buffer(8, 1):uint()
         -- dissect_XXX_cmd(buffer(9, len-9), len-9, pinfo, subtree)
         if cmdval == 0x01 -- set parameter
         or cmdval == 0x02 -- get parameter
         then
            if wrappedsize >= 10 then
               local scope = buffer(7, 1):uint() -- subprotocol
               local section = buffer(10, 1):uint()
               local addr = buffer(11, 1):uint()
               local subsubtree
               subtree:add(f_avr_parms_sect, buffer(10, 1))
               if scope == 0x01 and section == 0x00 then
                  subtree:add(f_avr_parms_gen_sect0, buffer(11, 1))
               elseif scope == 0x01 and section == 0x01 then
                  subtree:add(f_avr_parms_gen_sect1, buffer(11, 1))
               elseif scope == 0x01 and section == 0x02 then
                  subtree:add(f_avr_parms_gen_sect2, buffer(11, 1))
               elseif scope == 0x12 and section == 0x00 then
                  subtree:add(f_avr_parms_avr_sect0, buffer(11, 1))
               elseif scope == 0x12 and section == 0x01 then
                  subsubtree = subtree:add(f_avr_parms_avr_sect1, buffer(11, 1))
               elseif scope == 0x12 and section == 0x03 then
                  subtree:add(f_avr_parms_avr_sect1, buffer(11, 1))
               end
               subtree:add(f_avr_parms_nbytes, buffer(12, 1))
               local nbytes = buffer(12, 1):uint()
               if cmdval == 0x01 -- set
               and wrappedsize - 10 >= nbytes then
                  subsubtree = subtree:add(f_avr_parms_bytes, buffer(13, nbytes))
                  if scope == 0x12 and section == 0x00 then
                     if addr == 0x00 then
                        subsubtree:add(f_avr_parms_avr_arch, buffer(13,1))
                     elseif addr == 0x01 then
                        subsubtree:add(f_avr_parms_avr_session, buffer(13,1))
                     end
                  elseif scope == 0x12 and section == 0x01 then
                     if addr == 0x00 then
                        subsubtree:add(f_avr_parms_avr_connection, buffer(13,1))
                     elseif addr >= 0x20 then
                        -- clock values
                        subsubtree:add_le(f_avr_parms_avr_clock, buffer(13, 2))
                     end
                  end
               end
            end
         elseif cmdval == 0x21 -- read memory
         or cmdval == 0x23 -- write memory
         then
            if wrappedsize >= 18 then
               subtree:add(f_avr_mem_type, buffer(10, 1))
               subtree:add_le(f_avr_mem_addr, buffer(11, 4))
               subtree:add_le(f_avr_mem_size, buffer(15, 4))
               local nbytes = buffer(15, 4):le_uint()
               if cmdval == 0x23 and wrappedsize - 17 >= nbytes then
                  -- subtree:add_le(f_avr_mem_write_sync, buffer(19, 1))
                  subtree:add(f_avr_mem_bytes, buffer(20, nbytes))
               end
            end
         end
      end
   end

   function dissect_avr_cmd_rsp(buffer, len, pinfo, tree)
      if len >= 9 then
         if verbose then tree:add(f_avr_fragment_info, buffer(0, 1)) end
         tree:add(f_avr_size, buffer(1, 2))
         local wrappedsize = buffer(1, 2):uint()
         local subtree = tree:add(f_avr_packet, buffer(3, wrappedsize))
         if verbose then subtree:add(f_avr_sof, buffer(3, 1)) end
         subtree:add_le(f_avr_seq_id, buffer(4, 2))
         subtree:add(f_avr_subproto_id, buffer(6, 1))
         subtree = subtree:add(f_avr_rsp_code, buffer(7, 1))
         if verbose then subtree:add(f_avr_proto_version, buffer(8, 1)) end
         if len > 9 and wrappedsize > 6 then
            local rspcode = buffer(7, 1):uint()
            if rspcode == 0x80 then
               -- OK; no additional data expected
            elseif rspcode == 0x81 or rspcode == 0x84 then
               -- Info or Data
               subtree:add(f_avr_rsp_contents, buffer(9, wrappedsize - 7))
               subtree:add(f_avr_rsp_status, buffer(wrappedsize + 2, 1))
            elseif rspcode == 0x83 then
               -- PC
               if len >= 13 then
                  subtree:add_le(f_avr_pc, buffer(9, 4))
               end
            elseif rspcode == 0x84 then
               -- Failed
               subtree:add(f_avr_rsp_failure_code, buffer(9, 1))
            end
         end
      end
   end

   function dissect_avr_cmd_evt(buffer, len, pinfo, tree)
      if len >= 8 then
         tree:add(f_avr_size, buffer(1, 2))
         local wrappedsize = buffer(1, 2):uint()
         local subtree = tree:add(f_avr_packet, buffer(3, wrappedsize))
         if verbose then subtree:add(f_avr_sof, buffer(3, 1)) end
         subtree:add_le(f_avr_seq_id, buffer(4, 2))
         subtree:add(f_avr_subproto_id, buffer(6, 1))
         subtree = subtree:add(f_avr_evt_code, buffer(7, 1))
         if verbose then subtree:add(f_avr_proto_version, buffer(8, 1)) end
         local wrappedsize = buffer(1, 2):uint()
         if len > 9 and wrappedsize > 6 then
            local evtcode = buffer(7, 1):uint()
            if evtcode == 0x10 then
               -- power event
               subtree:add(f_avr_power_state, buffer(9, 1))
            elseif evtcode == 0x11 then
               -- sleep event
               subtree:add(f_avr_sleep_state, buffer(9, 1))
            elseif evtcode == 0x12 then
               -- external reset event
               subtree:add(f_avr_reset_state, buffer(9, 1))
            elseif evtcode == 0x40 then
               -- break event
               subtree:add_le(f_avr_pc, buffer(9, 4))
               subtree:add(f_avr_break_cause, buffer(13, 1))
               if verbose then subtree:add(f_avr_ext_info, buffer(15, 2)) end
            elseif evtcode == 0x41 then
               subtree:add(f_avr_idr, buffer(9, 1))
            end
         end
      end
   end

   -- Init function, called before any packet is dissected
   function edbg_proto.init()
      -- print("edbg.init")
   end

   -- Main EDBG dissector
   function edbg_proto.dissector(buffer, pinfo, tree)
      if buffer:len() > 64 then
         local data = buffer(64, buffer:len() - 64)
         local dlen = data:len()
         pinfo.cols.protocol = "EDBG"
         local subtree = tree:add(edbg_proto, data(0, dlen),
                                  "EDBG Protocol Data")

         local dirval = dir().value
         subtree:add(f_dir, dirval)
         subtree = subtree:add(f_cmd, data(0, 1))

         local cmd = data(0, 1):uint()
         dlen = dlen - 1
         if dlen > 0 then
            data = data(1, dlen)
            if cmd == 0x00 then
               -- DAP_Info
               if dirval == 0 then
                  dissect_dap_info_req(data, dlen, pinfo, subtree)
               else
                  dissect_dap_info_rsp(data, dlen, pinfo, subtree)
               end
            elseif cmd == 0x01 then
               -- DAP_LED
               if dirval == 0 then
                  if dlen >= 2 then
                     subtree:add(f_dap_led_id, data(0, 1))
                     subtree:add(f_dap_led_status, data(1, 1))
                  end
               end
            elseif cmd == 0x02 then
               -- DAP_Connect
               if dirval == 0 then
                  subtree:add(f_dap_port, data(0, 1))
               else
                  subtree:add(f_dap_port_status, data(0, 1))
               end
            elseif cmd == 0x03 then
               -- DAP_Disconnect
               if dirval ~= 0 then
                  subtree:add(f_status, data(0, 1))
               end
            elseif cmd == 0x04 then
               -- DAP_TransferConfigure
               if dirval == 0 then
                  if dlen >= 5 then
                     subtree:add(f_xfr_idle_cycles, data(0, 1))
                     subtree:add_le(f_xfr_wait_retry, data(1, 2))
                     subtree:add_le(f_xfr_match_retry, data(3, 2))
                  end
               else
                  subtree:add(f_status, data(0, 1))
               end
            elseif cmd == 0x11 then
               -- DAP_SWJ_Clock
               if dirval == 0 then
                  if dlen >= 4 then
                     subtree:add_le(f_dap_swj_clock, data(0, 4))
                  end
               else
                  subtree:add(f_status, data(0, 1))
               end
            elseif cmd == 0x80 then
               -- AVR_CMD
               if dirval == 0 then
                  dissect_avr_cmd_req(data, dlen, pinfo, subtree)
               else
                  if verbose then
                     subtree:add(f_avr_fragment_code, data(0, 1))
                  end
               end
            elseif cmd == 0x81 then
               -- AVR_RSP
               if dirval ~= 0 then
                  dissect_avr_cmd_rsp(data, dlen, pinfo, subtree)
               end
            elseif cmd == 0x82 then
               -- AVR_Event
               if dirval ~= 0 then
                  dissect_avr_cmd_evt(data, dlen, pinfo, subtree)
               end
            end
         end
      end
   end

   -- Create protocol fields
   edbg_proto.fields = {
      f_cmd,
      f_dir,
      f_status,

      f_dap_info_id,
      f_dap_info_string,
      f_dap_info_byte,
      f_dap_info_short,

      f_dap_led_id,
      f_dap_led_status,

      f_dap_port,
      f_dap_port_status,

      f_dap_swj_clock,

      f_xfr_idle_cycles,
      f_xfr_wait_retry,
      f_xfr_match_retry,

      f_avr_fragment_info,
      f_avr_size,
      f_avr_fragment_code,
      f_avr_packet,

      f_avr_sof,
      f_avr_proto_version,
      f_avr_seq_id,
      f_avr_subproto_id,

      f_avr_cmd,
      f_avr_rsp_code,
      f_avr_rsp_contents,
      f_avr_pc,
      f_avr_rsp_failure_code,
      f_avr_rsp_status,

      f_avr_evt_code,
      f_avr_power_state,
      f_avr_sleep_state,
      f_avr_reset_state,
      f_avr_break_cause,
      f_avr_idr,
      f_avr_ext_info,

      f_avr_parms_sect,
      f_avr_parms_gen_sect0,
      f_avr_parms_gen_sect1,
      f_avr_parms_gen_sect2,
      f_avr_parms_avr_sect0,
      f_avr_parms_avr_sect1,
      f_avr_parms_avr_sect3,
      f_avr_parms_avr_arch,
      f_avr_parms_avr_session,
      f_avr_parms_avr_connection,
      f_avr_parms_nbytes,
      f_avr_parms_bytes,
      f_avr_parms_avr_clock,

      f_avr_mem_type,
      f_avr_mem_addr,
      f_avr_mem_size,
      f_avr_mem_bytes
   }

   register_postdissector(edbg_proto)

-- end

-- (This is for Emacs users.)
-- Local Variables:
-- indent-tabs-mode: nil
-- End:
