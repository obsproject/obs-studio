<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0"
xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
xmlns:sparkle="http://www.andymatuschak.org/xml-namespaces/sparkle">
<xsl:output method="xml" encoding="UTF-8" omit-xml-declaration="no"/>
<xsl:strip-space elements="*"/>

<!-- Select /rss/channel/title and store it as default value for 'pCustomTitle' -->
<xsl:param name="pCustomTitle" select="/rss/channel/title" />
<!-- Select /rss/channel/link and store it as default value for 'pCustomLink' -->
<xsl:param name="pCustomLink" select="/rss/channel/link" />
<!-- Set empty strings as default values for pSparkleUrl and pDeltaUrl -->
<xsl:param name="pSparkleUrl" select="''" />
<xsl:param name="pDeltaUrl" select="''" />

<!-- XSLT identity rule - copy all nodes and their child nodes as well as attributes
     (attributes are _not_ descendants of the nodes they belong to).
     This copy rule is applied as the "default" transformation for all nodes.
-->
<xsl:template match="@* | node()">
    <xsl:copy>
        <xsl:apply-templates select="@* | node()" />
    </xsl:copy>
</xsl:template>
<!-- Match the <title> under <channel> and <rss> and do not translate it
     (effectively removes it).
-->
<xsl:template match="/rss/channel/title" />
<!-- Match the <link> under <channel> and <rss> and do not translate it
     (effectively removes it).
 -->
<xsl:template match="/rss/channel/link" />
<!-- Match the <channel> under <rss> and apply a copy translation, which
     * Creates a new <title> element with a text child node and the text content
       of the pCustomTitle variable
     * Creates a new <link> element with a text child node and the text content
       of the pCustomLink variable
     * Copies all child nodes and attributes of the original <channel> node
       (this is why <title> and <link> were explicitly not translated before)
-->
<xsl:template match="/rss/channel">
    <xsl:copy>
        <xsl:element name="title"><xsl:value-of select="$pCustomTitle" /></xsl:element>
        <xsl:element name="link"><xsl:value-of select="$pCustomLink" /></xsl:element>
        <xsl:apply-templates select="@* | node()" />
    </xsl:copy>
</xsl:template>
<!-- Match every url attribute of <enclosure> nodes in <sparkle:deltas> nodes
     (which themselves are under <item>, <channel>, and <rss> nodes respectively).

     Create a new attribute with the name "url" and then set its content to either
     * The original value of the attribute if it starts with the value of the
       pDeltaUrl variable, OR
     * The actual value of the pDeltaUrl variable, with the value of the
       pSparkleUrl variable removed in front of the current url value added after

     This effectively updates every url attribute on a delta <enclosure> node
     that does not start with the current delta url path.
-->
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
<!-- Match any <sparkle:fullReleaseNotesLink> node under <item>, <channel>,
     and <rss> respectively, and replace it with a new node named
     <sparkle:releaseNotesLink> and populate it with all child nodes and
        attributes of the original node.
-->
<xsl:template match="/rss/channel/item/sparkle:fullReleaseNotesLink">
    <xsl:element name="sparkle:releaseNotesLink"><xsl:apply-templates select="@* | node()" /></xsl:element>
</xsl:template>
</xsl:stylesheet>
