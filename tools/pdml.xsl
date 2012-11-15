<?xml version="1.0" encoding='ISO-8859-1' ?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<!--
 * Copyright (c) 2012 Joerg Wunsch
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
 * Extract all the interesting (for us) USB packet details from a
 * Wireshark "packet detail" XML file (PDML).  Can be used to
 * analyze USB data which have been sniffed by Wireshark.  Just
 * export them in PDML format there, and run this stylesheet across
 * them.
-->
    <xsl:output method="text"/>

    <xsl:template match="/">

      <xsl:for-each select='//pdml/packet/proto/field'>

      <xsl:if test='@name="usb.endpoint_number"'>
	<xsl:if test='@show!="0x80"'>
	  <!-- ignore the CONTROL endpoint data -->
          <xsl:value-of select='@show' />

	  <xsl:for-each select='field'>
	    <xsl:if test='@name="usb.endpoint_number.direction"'>
	      <xsl:choose>
		<xsl:when test='@show="1"'>
		  <!-- IN transfer -->
		  <xsl:text> &lt;-- </xsl:text>
		</xsl:when>
		<xsl:otherwise>
		  <!-- OUT transver -->
		  <xsl:text> --&gt; </xsl:text>
		</xsl:otherwise>
	      </xsl:choose>
	    </xsl:if>
	  </xsl:for-each>
	</xsl:if>

      </xsl:if>

      <xsl:if test='@name="usb.capdata"'>
	<xsl:call-template name="format-hex">
	  <xsl:with-param name="arg" select="@value" />
	  <xsl:with-param name="count" select="0" />
	</xsl:call-template>
        <xsl:text>&#10;</xsl:text>
      </xsl:if>

      </xsl:for-each>

    </xsl:template>

    <!-- Format a string of hex numbers: start a new line -->
    <!-- after each 8 hex numbers -->
    <!-- call with parameter $count = 0, calls itself -->
    <!-- recursively then until everything has been done -->
    <xsl:template name="format-hex">
      <xsl:param name="arg" />
      <xsl:param name="count" />
      <xsl:choose>
        <xsl:when test="string-length($arg) &lt;= 2">
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
          <xsl:value-of select="substring($arg, 1, 2)" />
          <xsl:choose>
            <xsl:when test="$count mod 16 = 15">
              <xsl:text> &#010;         </xsl:text>
            </xsl:when>
            <xsl:otherwise>
              <xsl:text> </xsl:text>
            </xsl:otherwise>
          </xsl:choose>
          <xsl:call-template name="format-hex">
            <xsl:with-param name="arg" select="substring($arg, 3)" />
            <xsl:with-param name="count" select="$count + 1" />
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:template>

</xsl:stylesheet>
