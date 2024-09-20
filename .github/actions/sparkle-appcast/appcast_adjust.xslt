<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0"
xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
xmlns:sparkle="http://www.andymatuschak.org/xml-namespaces/sparkle">
<xsl:output method="xml" encoding="UTF-8" omit-xml-declaration="no"/>
<xsl:strip-space elements="*"/>

<xsl:param name="pCustomTitle" select="/rss/channel/title" />
<xsl:param name="pCustomLink" select="/rss/channel/link" />
<xsl:param name="pSparkleUrl" select="''" />
<xsl:param name="pDeltaUrl" select="''" />

<xsl:template match="@* | node()">
    <xsl:copy>
        <xsl:apply-templates select="@* | node()" />
    </xsl:copy>
</xsl:template>
<xsl:template match="/rss/channel/title" />
<xsl:template match="/rss/channel/link" />
<xsl:template match="/rss/channel">
    <xsl:copy>
        <xsl:element name="title"><xsl:value-of select="$pCustomTitle" /></xsl:element>
        <xsl:element name="link"><xsl:value-of select="$pCustomLink" /></xsl:element>
        <xsl:apply-templates select="@* | node()" />
    </xsl:copy>
</xsl:template>
<xsl:template match="/rss/channel/item/sparkle:deltas/enclosure/@url">
    <xsl:attribute name="url">
        <xsl:choose>
            <xsl:when test="starts-with(., $pDeltaUrl)">
                <xsl:value-of select="." />
            </xsl:when>
            <xsl:otherwise>
                <xsl:value-of select="$pDeltaUrl" />
                <xsl:value-of select="substring-after(., $pSparkleUrl)" />
            </xsl:otherwise>
        </xsl:choose>
    </xsl:attribute>
</xsl:template>
<xsl:template match="/rss/channel/item/sparkle:fullReleaseNotesLink">
    <xsl:element name="sparkle:releaseNotesLink"><xsl:apply-templates select="@* | node()" /></xsl:element>
</xsl:template>
</xsl:stylesheet>
