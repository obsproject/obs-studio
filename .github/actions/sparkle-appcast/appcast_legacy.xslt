<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0"
xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
xmlns:sparkle="http://www.andymatuschak.org/xml-namespaces/sparkle">
<xsl:output method="xml" encoding="UTF-8" omit-xml-declaration="no"/>
<xsl:strip-space elements="*"/>

<xsl:template match="@* | node()">
    <xsl:copy>
        <xsl:apply-templates select="@* | node()" />
    </xsl:copy>
</xsl:template>
<xsl:template match="/rss/channel/item[sparkle:channel[text()!='stable']]" />
<xsl:template match="/rss/channel/item/sparkle:channel" />
<xsl:template match="/rss/channel/item/sparkle:deltas" />
</xsl:stylesheet>
