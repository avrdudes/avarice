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
 * Extract OCD_REVISION and OSCCAL out of Atmel Studio 6.x device XML files.
-->
    <xsl:output method="text"/>

    <xsl:template match="//property[@name='OCD_REVISION'][//register[@name='OSCCAL']]">

      <xsl:value-of select="//device/@name" />
      <xsl:text>&#9;</xsl:text>
      <xsl:value-of select="//register[@name='OSCCAL']/@offset" />
      <xsl:text>&#9;</xsl:text>
      <xsl:value-of select="//property[@name='OCD_REVISION']/@value" />
      <xsl:text>&#10;</xsl:text>

    </xsl:template>

    <xsl:template match="node()">
      <xsl:apply-templates />
    </xsl:template>

    <xsl:template match="/">
      <xsl:apply-templates />
    </xsl:template>

</xsl:stylesheet>
