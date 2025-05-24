<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0"
xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
xmlns:sparkle="http://www.andymatuschak.org/xml-namespaces/sparkle">
<xsl:output method="xml" encoding="UTF-8" omit-xml-declaration="no"/>
<xsl:strip-space elements="*"/>

<!-- XSLT identity rule - copy all nodes and their child nodes as well as attributes
     (attributes are _not_ descendants of the nodes they belong to).
     This copy rule is applied as the "default" transformation for all nodes.
-->
<xsl:template match="@* | node()">
    <xsl:copy>
        <xsl:apply-templates select="@* | node()" />
    </xsl:copy>
</xsl:template>
<!-- Select every <item> node under a <channel> and <rss> node respectively,
     which has a <sparkle:channel> child node whose text child node's value
     is not equal to 'stable', then apply to translation, effectively removing
     it.
-->
<xsl:template match="/rss/channel/item[sparkle:channel[text()!='stable']]" />
<!-- Select every <sparkle:channel> node under a <item> node which sits under a
     <channel> and <rss> node respectively, then apply no translation, effectively
     removing it.
-->
<xsl:template match="/rss/channel/item/sparkle:channel" />
<!-- Select every <sparkle:deltas> node under a <item> node which sits under a
     <channel> and <rss> node respectively, then apply no translation, effectively
     removing it.
-->
<xsl:template match="/rss/channel/item/sparkle:deltas" />
</xsl:stylesheet>
