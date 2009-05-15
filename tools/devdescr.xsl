<?xml version="1.0" encoding='ISO-8859-1' ?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<!--
 * Copyright (c) 2006 Joerg Wunsch
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 *
 * $Id$
-->
<!--
 * Extract all the data needed for the JTAG ICE mkII device decriptor
 * from the XML, and format it the way src/devdescr.cc needs it.
 *
 * This file will *only* generate JTAG ICE mkII entries.
 *
 * Run this file together with the respective AVR's XML file through
 * an XSLT processor (xsltproc, saxon), and capture the output for
 * inclusion into src/devdescr.cc.
-->
    <xsl:output method="text"/>
    <xsl:template match="/">
      <!-- Extract everything we need out of the XML. -->
      <xsl:variable name="devname"
                    select="translate(/AVRPART/ADMIN/PART_NAME,
                                      'abcdefghijklmnopqrstuvwxyz',
                                      'ABCDEFGHIJKLMNOPQRSTUVWXYZ')" />
      <xsl:variable name="devname_lower"
                    select="translate(/AVRPART/ADMIN/PART_NAME,
                                      'ABCDEFGHIJKLMNOPQRSTUVWXYZ',
                                      'abcdefghijklmnopqrstuvwxyz')" />
      <xsl:variable name="sig"
                    select="concat(substring(/AVRPART/ADMIN/SIGNATURE/ADDR001, 2),
                                   substring(/AVRPART/ADMIN/SIGNATURE/ADDR002, 2))" />
      <xsl:variable name="flashsize"
                    select="/AVRPART/MEMORY/PROG_FLASH" />
      <xsl:variable name="eepromsize"
                    select="/AVRPART/MEMORY/EEPROM" />
      <xsl:variable name="flashpagesize"
                    select="/AVRPART/PROGRAMMING/FlashPageSize" />
      <xsl:variable name="eeprompagesize">
        <xsl:choose>
          <xsl:when test="/AVRPART/PROGRAMMING/EepromPageSize = 0">
            <xsl:text>1</xsl:text>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="/AVRPART/PROGRAMMING/EepromPageSize" />
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <xsl:variable name="nmb_vectors"
                    select="/AVRPART/INTERRUPT_VECTOR/NMB_VECTORS" />
      <xsl:variable name="sramstart"
                    select="/AVRPART/MEMORY/INT_SRAM/START_ADDR" />
      <xsl:variable name="SPH"
                    select="/AVRPART/MEMORY/IO_MEMORY/SPH/IO_ADDR" />
      <xsl:variable name="SPL"
                    select="/AVRPART/MEMORY/IO_MEMORY/SPL/IO_ADDR" />
      <xsl:variable name="RAMPZ"
                    select="/AVRPART/MEMORY/IO_MEMORY/RAMPZ/IO_ADDR" />
      <xsl:variable name="EIND"
                    select="/AVRPART/MEMORY/IO_MEMORY/EIND/IO_ADDR" />
      <xsl:variable name="EECR"
                    select="/AVRPART/MEMORY/IO_MEMORY/EECR/IO_ADDR" />
      <xsl:variable name="ucRead"
                    select="//AVRPART/ICE_SETTINGS/JTAGICEmkII/ucRead" />
      <xsl:variable name="ucWrite"
                    select="//AVRPART/ICE_SETTINGS/JTAGICEmkII/ucWrite" />
      <xsl:variable name="ucReadShadow"
                    select="//AVRPART/ICE_SETTINGS/JTAGICEmkII/ucReadShadow" />
      <xsl:variable name="ucWriteShadow"
                    select="//AVRPART/ICE_SETTINGS/JTAGICEmkII/ucWriteShadow" />
      <xsl:variable name="ucExtRead"
                    select="//AVRPART/ICE_SETTINGS/JTAGICEmkII/ucExtRead" />
      <xsl:variable name="ucExtWrite"
                    select="//AVRPART/ICE_SETTINGS/JTAGICEmkII/ucExtWrite" />
      <xsl:variable name="ucExtReadShadow"
                    select="//AVRPART/ICE_SETTINGS/JTAGICEmkII/ucExtReadShadow" />
      <xsl:variable name="ucExtWriteShadow"
                    select="//AVRPART/ICE_SETTINGS/JTAGICEmkII/ucExtWriteShadow" />
      <xsl:variable name="ucIDRAddress"
                    select="//AVRPART/ICE_SETTINGS/JTAGICEmkII/ucIDRAddress" />
      <xsl:variable name="ucSPMCAddress"
                    select="//AVRPART/ICE_SETTINGS/JTAGICEmkII/ucSPMCAddress" />
      <xsl:variable name="ulBootAddress"
                    select="//AVRPART/ICE_SETTINGS/JTAGICEmkII/ulBootAddress" />
      <xsl:variable name="ucUpperExtIOLoc"
                    select="//AVRPART/ICE_SETTINGS/JTAGICEmkII/ucUpperExtIOLoc" />
      <xsl:variable name="ucEepromInst"
                    select="//AVRPART/ICE_SETTINGS/JTAGICEmkII/ucEepromInst" />
      <xsl:variable name="ucFlashInst"
                    select="//AVRPART/ICE_SETTINGS/JTAGICEmkII/ucFlashInst" />
      <xsl:variable name="ucSPHaddr"
                    select="//AVRPART/ICE_SETTINGS/JTAGICEmkII/ucSPHaddr" />
      <xsl:variable name="ucSPLaddr"
                    select="//AVRPART/ICE_SETTINGS/JTAGICEmkII/ucSPLaddr" />
      <xsl:variable name="DWdatareg"
                    select="//AVRPART/ICE_SETTINGS/JTAGICEmkII/DWdatareg" />
      <xsl:variable name="DWbasePC"
                    select="//AVRPART/ICE_SETTINGS/JTAGICEmkII/DWbasePC" />
      <xsl:variable name="ucAllowFullPageBitstream"
                    select="//AVRPART/ICE_SETTINGS/JTAGICEmkII/ucAllowFullPageBitstream" />
      <xsl:variable name="uiStartSmallestBootLoaderSection"
                    select="//AVRPART/ICE_SETTINGS/JTAGICEmkII/uiStartSmallestBootLoaderSection" />
      <xsl:variable name="CacheType"
                    select="//AVRPART/ICE_SETTINGS/JTAGICEmkII/CacheType" />
      <xsl:variable name="ResetType"
                    select="//AVRPART/ICE_SETTINGS/JTAGICEmkII/ResetType" />
      <xsl:variable name="PCMaskExtended"
                    select="//AVRPART/ICE_SETTINGS/JTAGICEmkII/PCMaskExtended" />
      <xsl:variable name="PCMaskHigh"
                    select="//AVRPART/ICE_SETTINGS/JTAGICEmkII/PCMaskHigh" />

      <!-- If there's a JTAGICEmkII node at all, emit the descriptor. -->
      <xsl:if test='//AVRPART/ICE_SETTINGS/JTAGICEmkII'>

      <!-- start of new entry -->
      <xsl:text>    // DEV_</xsl:text>
      <xsl:value-of select="$devname" />
      <xsl:text>&#010;    {&#010;&#009;&quot;</xsl:text>
      <xsl:value-of select="$devname_lower" />
      <xsl:text>&quot;,&#010;&#009;0x</xsl:text>

      <!-- signature -->
      <xsl:value-of select="$sig" />
      <xsl:text>,&#010;</xsl:text>

      <!-- flash page size and size -->
      <xsl:text>&#009;</xsl:text>
      <xsl:value-of select="$flashpagesize" />
      <xsl:text>, </xsl:text>
      <xsl:value-of select="$flashsize div $flashpagesize" />
      <xsl:text>,&#009;// </xsl:text>
      <xsl:value-of select="$flashsize" />
      <xsl:text> bytes flash&#010;</xsl:text>

      <!-- EEPROM page size and size -->
      <xsl:text>&#009;</xsl:text>
      <xsl:value-of select="$eeprompagesize" />
      <xsl:text>, </xsl:text>
      <xsl:value-of select="$eepromsize div $eeprompagesize" />
      <xsl:text>,&#009;// </xsl:text>
      <xsl:value-of select="$eepromsize" />
      <xsl:text> bytes EEPROM&#010;</xsl:text>

      <!-- interrupt vector size -->
      <xsl:text>&#009;</xsl:text>
      <xsl:value-of select="$nmb_vectors" />
      <xsl:text> * </xsl:text>
      <xsl:choose>
        <xsl:when test="$flashsize &gt; 8192">
          <xsl:text>4</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>2</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:text>,&#009;// </xsl:text>
      <xsl:value-of select="$nmb_vectors" />
      <xsl:text> interrupt vectors&#010;</xsl:text>

      <!-- static stuff -->
      <xsl:text>&#009;DEVFL_MKII_ONLY,&#010;</xsl:text>
      <xsl:text>&#009;NULL,&#009;// registers not yet defined&#010;</xsl:text>

      <!-- Xmega? -->
      <xsl:text>&#009;</xsl:text>
      <xsl:choose>
        <xsl:when test="substring(//AVRPART/ICE_SETTINGS/JTAGICEmkII/Interface,1,5) = 'Xmega'">
          <xsl:text>true</xsl:text>
        </xsl:when>
        <xsl:otherwise> <!-- i.e., JTAG -->
          <xsl:text>false</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:text>,&#010;</xsl:text>

      <!-- more static stuff -->
      <xsl:text>&#009;{&#010;&#009;    0&#009;// no mkI support&#010;</xsl:text>
      <xsl:text>&#009;},&#010;&#009;{&#010;&#009;</xsl:text>
      <xsl:text>    CMND_SET_DEVICE_DESCRIPTOR,&#010;</xsl:text>

      <!-- IO register read and write bit definitions -->
      <xsl:text>&#009;    { </xsl:text>
      <xsl:value-of select="$ucRead" />
      <xsl:text> }, // ucReadIO&#010;</xsl:text>
      <xsl:text>&#009;    { </xsl:text>
      <xsl:value-of select="$ucReadShadow" />
      <xsl:text> }, // ucReadIOShadow&#010;</xsl:text>
      <xsl:text>&#009;    { </xsl:text>
      <xsl:value-of select="$ucWrite" />
      <xsl:text> }, // ucWriteIO&#010;</xsl:text>
      <xsl:text>&#009;    { </xsl:text>
      <xsl:value-of select="$ucWriteShadow" />
      <xsl:text> }, // ucWriteIOShadow&#010;</xsl:text>
      <xsl:text>&#009;    { </xsl:text>
      <!-- The extIO strings are long, so use format-hex. -->
      <xsl:call-template name="format-hex">
        <xsl:with-param name="arg" select="$ucExtRead" />
        <xsl:with-param name="count" select="0" />
      </xsl:call-template>
      <xsl:text> }, // ucReadExtIO&#010;</xsl:text>
      <xsl:text>&#009;    { </xsl:text>
      <xsl:call-template name="format-hex">
        <xsl:with-param name="arg" select="$ucExtReadShadow" />
        <xsl:with-param name="count" select="0" />
      </xsl:call-template>
      <xsl:text> }, // ucReadIOExtShadow&#010;</xsl:text>
      <xsl:text>&#009;    { </xsl:text>
      <xsl:call-template name="format-hex">
        <xsl:with-param name="arg" select="$ucExtWrite" />
        <xsl:with-param name="count" select="0" />
      </xsl:call-template>
      <xsl:text> }, // ucWriteExtIO&#010;</xsl:text>
      <xsl:text>&#009;    { </xsl:text>
      <xsl:call-template name="format-hex">
        <xsl:with-param name="arg" select="$ucExtWriteShadow" />
        <xsl:with-param name="count" select="0" />
      </xsl:call-template>
      <xsl:text> }, // ucWriteIOExtShadow&#010;</xsl:text>

      <!-- miscellaneous -->
      <xsl:text>&#009;    </xsl:text>
      <xsl:value-of select="$ucIDRAddress" />
      <xsl:text>,&#009;// ucIDRAddress&#010;</xsl:text>

      <xsl:text>&#009;    </xsl:text>
      <xsl:value-of select="$ucSPMCAddress" />
      <xsl:text>,&#009;// ucSPMCRAddress&#010;</xsl:text>

      <xsl:text>&#009;    </xsl:text>
      <xsl:call-template name="dollar-to-0x">
        <xsl:with-param name="arg" select="$RAMPZ" />
      </xsl:call-template>
      <xsl:text>,&#009;// ucRAMPZAddress&#010;</xsl:text>

      <xsl:text>&#009;    fill_b2(</xsl:text>
      <xsl:value-of select="$flashpagesize" />
      <xsl:text>),&#009;// uiFlashPageSize&#010;</xsl:text>

      <xsl:text>&#009;    </xsl:text>
      <xsl:value-of select="$eeprompagesize" />
      <xsl:text>,&#009;// ucEepromPageSize&#010;</xsl:text>

      <xsl:text>&#009;    fill_b4(</xsl:text>
      <xsl:value-of select="$ulBootAddress" />
      <xsl:text>),&#009;// ulBootAddress&#010;</xsl:text>

      <xsl:text>&#009;    fill_b2(</xsl:text>
      <xsl:value-of select="$ucUpperExtIOLoc" />
      <xsl:text>),&#009;// uiUpperExtIOLoc&#010;</xsl:text>

      <xsl:text>&#009;    fill_b4(</xsl:text>
      <xsl:value-of select="$flashsize" />
      <xsl:text>),&#009;// ulFlashSize&#010;</xsl:text>

      <xsl:text>&#009;    { </xsl:text>
      <xsl:call-template name="format-hex">
        <xsl:with-param name="arg" select="$ucEepromInst" />
        <xsl:with-param name="count" select="0" />
      </xsl:call-template>
      <xsl:text> },&#009;// ucEepromInst&#010;</xsl:text>

      <xsl:text>&#009;    { </xsl:text>
      <xsl:call-template name="format-hex">
        <xsl:with-param name="arg" select="$ucFlashInst" />
        <xsl:with-param name="count" select="0" />
      </xsl:call-template>
      <xsl:text> },&#009;// ucFlashInst&#010;</xsl:text>

      <xsl:text>&#009;    </xsl:text>
      <xsl:call-template name="dollar-to-0x">
        <xsl:with-param name="arg" select="$SPH" />
      </xsl:call-template>

      <xsl:text>,&#009;// ucSPHaddr&#010;</xsl:text>
      <xsl:text>&#009;    </xsl:text>
      <xsl:call-template name="dollar-to-0x">
        <xsl:with-param name="arg" select="$SPL" />
      </xsl:call-template>
      <xsl:text>,&#009;// ucSPLaddr&#010;</xsl:text>

      <xsl:text>&#009;    fill_b2(</xsl:text>
      <xsl:value-of select="$flashsize" />
      <xsl:text> / </xsl:text>
      <xsl:value-of select="$flashpagesize" />
      <xsl:text>),&#009;// uiFlashpages&#010;</xsl:text>

      <xsl:text>&#009;    </xsl:text>
      <xsl:value-of select="$DWdatareg" />
      <xsl:text>,&#009;// ucDWDRAddress&#010;</xsl:text>

      <xsl:text>&#009;    </xsl:text>
      <xsl:value-of select="$DWbasePC" />
      <xsl:text>,&#009;// ucDWBasePC&#010;</xsl:text>

      <xsl:text>&#009;    </xsl:text>
      <xsl:value-of select="$ucAllowFullPageBitstream" />
      <xsl:text>,&#009;// ucAllowFullPageBitstream&#010;</xsl:text>

      <xsl:text>&#009;    fill_b2(</xsl:text>
      <xsl:value-of select="$uiStartSmallestBootLoaderSection" />
      <xsl:text>),&#009;// uiStartSmallestBootLoaderSection&#010;</xsl:text>

      <!-- EnablePageProgramming is not always given in the XML files. -->
      <!-- All our target processors are organized in pages though. -->
      <xsl:text>&#009;    1,&#009;// EnablePageProgramming&#010;</xsl:text>

      <xsl:text>&#009;    </xsl:text>
      <xsl:call-template name="maybezero">
        <xsl:with-param name="arg" select="$CacheType" />
      </xsl:call-template>
      <xsl:text>,&#009;// ucCacheType&#010;</xsl:text>

      <xsl:text>&#009;    fill_b2(</xsl:text>
      <xsl:call-template name="dollar-to-0x">
        <xsl:with-param name="arg" select="$sramstart" />
      </xsl:call-template>
      <xsl:text>),&#009;// uiSramStartAddr&#010;</xsl:text>

      <xsl:text>&#009;    </xsl:text>
      <xsl:call-template name="maybezero">
        <xsl:with-param name="arg" select="$ResetType" />
      </xsl:call-template>
      <xsl:text>,&#009;// ucResetType&#010;</xsl:text>

      <xsl:text>&#009;    </xsl:text>
      <xsl:call-template name="maybezero">
        <xsl:with-param name="arg" select="$PCMaskExtended" />
      </xsl:call-template>
      <xsl:text>,&#009;// ucPCMaskExtended&#010;</xsl:text>

      <xsl:text>&#009;    </xsl:text>
      <xsl:call-template name="maybezero">
        <xsl:with-param name="arg" select="$PCMaskHigh" />
      </xsl:call-template>
      <xsl:text>,&#009;// ucPCMaskHigh&#010;</xsl:text>

      <xsl:text>&#009;    </xsl:text>
      <xsl:call-template name="dollar-to-0x">
        <xsl:with-param name="arg" select="$EIND" />
      </xsl:call-template>
      <xsl:text>,&#009;// ucEindAddress&#010;</xsl:text>

      <xsl:text>&#009;    fill_b2(</xsl:text>
      <xsl:call-template name="dollar-to-0x">
        <xsl:with-param name="arg" select="$EECR" />
      </xsl:call-template>
      <xsl:text>),&#009;// EECRAddress&#010;</xsl:text>

      <!-- trailer -->
      <xsl:text>&#009;},&#010;</xsl:text>
      <xsl:text>    },&#010;</xsl:text>

      </xsl:if> <!-- JTAGICEmkII is present -->

    </xsl:template>

    <xsl:template name="toupper">
    </xsl:template>

    <!-- return argument $arg if non-empty, 0 otherwise -->
    <xsl:template name="maybezero">
      <xsl:param name="arg" />
      <xsl:choose>
        <xsl:when test="string-length($arg) = 0"><xsl:text>0</xsl:text></xsl:when>
        <xsl:otherwise><xsl:value-of select="$arg" /></xsl:otherwise>
      </xsl:choose>
    </xsl:template> <!-- maybezero -->

    <!-- convert $XX hex number in $arg (if any) into 0xXX; -->
    <!-- return 0 if $arg is empty -->
    <xsl:template name="dollar-to-0x">
      <xsl:param name="arg" />
      <xsl:choose>
        <xsl:when test="string-length($arg) = 0">
          <xsl:text>0</xsl:text>
        </xsl:when>
        <xsl:when test="substring($arg, 1, 1) = '&#036;'">
          <xsl:text>0x</xsl:text>
          <xsl:value-of select="substring($arg, 2)" />
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$arg" />
        </xsl:otherwise>
      </xsl:choose>
    </xsl:template> <!-- dollar-to-0x -->

    <!-- Format a string of 0xXX numbers: start a new line -->
    <!-- after each 8 hex numbers -->
    <!-- call with parameter $count = 0, calls itself -->
    <!-- recursively then until everything has been done -->
    <xsl:template name="format-hex">
      <xsl:param name="arg" />
      <xsl:param name="count" />
      <xsl:choose>
        <xsl:when test="string-length($arg) &lt;= 4">
          <!-- Last element, print it, and leave template. -->
          <xsl:value-of select="$arg" />
        </xsl:when>
        <xsl:otherwise>
          <!--
            * More arguments follow, print first value,
            * followed by a comma, decide whether a space
            * or a newline needs to be emitted, and recurse
            * with the remaining part of $arg.
          -->
          <xsl:value-of select="substring($arg, 1, 4)" />
          <xsl:choose>
            <xsl:when test="$count mod 8 = 7">
              <xsl:text>,&#010;&#009;      </xsl:text>
            </xsl:when>
            <xsl:otherwise>
              <xsl:text>,</xsl:text>
            </xsl:otherwise>
          </xsl:choose>
          <xsl:variable name="newarg">
            <!-- see whether there is a space after comma -->
            <xsl:choose>
              <xsl:when test="substring($arg, 6, 1) = ' '">
                <xsl:value-of select="substring($arg, 7)" />
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="substring($arg, 6)" />
              </xsl:otherwise>
            </xsl:choose>
          </xsl:variable>
          <xsl:call-template name="format-hex">
            <xsl:with-param name="arg" select="$newarg" />
            <xsl:with-param name="count" select="$count + 1" />
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:template>

</xsl:stylesheet>
