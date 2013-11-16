<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" 
                xmlns:dc="http://purl.org/dc/elements/1.1/" 
                xmlns:fn="http://www.w3.org/2005/xpath-functions"
                xmlns:xs="http://www.w3.org/2001/XMLSchema" 
                xmlns:fox="http://xmlgraphics.apache.org/fop/extensions"
                xmlns:fo="http://www.w3.org/1999/XSL/Format" 
                xmlns:int="http://catcode.com/odf_to_xhtml/internal" 
                xmlns:math="http://www.w3.org/1998/Math/MathML" 
                xmlns:svg="urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0" 
                xmlns:xforms="http://www.w3.org/2002/xforms" 
                xmlns:xlink="http://www.w3.org/1999/xlink" 
                xmlns:cms="http://www.freeroad.ch/2013/CMSFormat" 
                version="1.0">
    
    <xsl:output method="xml"
                indent="yes"
                omit-xml-declaration="yes"
                encoding="UTF-8" />
    
    
    <!--  doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN"
                doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd"  -->

    <!-- pdf2html.xsl  variable start <xsl:output method="html" omit-xml-declaration="yes" encoding="utf-8"/>  -->  
    <xsl:param name="Convert_Time" select="'0'"/>
    
    <xsl:variable name="PrependTagClass" select="'fox'"/>
    <xsl:variable name="PrependImage" select="'REFIMAGE='"/>
    <xsl:variable name="Title_Document" select="/pdf2xml/@doctitle"/>
    <xsl:variable name="Filename" select="/pdf2xml/@docname"/>
    <!-- param incomming from xsltproc -->
    
    
    <xsl:variable name="lineBreak">
        <xsl:text>
        </xsl:text>
    </xsl:variable>
    <xsl:variable name="tabulator">
        <xsl:text>      </xsl:text>
    </xsl:variable>
    <xsl:variable name="tabtab" xml:space="preserve"><xsl:text>   </xsl:text></xsl:variable>
    <xsl:variable name="debugmy" select="'0'"></xsl:variable>
    <xsl:variable name="onlymastercss" select="'0'"></xsl:variable>
    <xsl:variable name="debugcss" select="'0'"></xsl:variable>
    <xsl:variable name="PageWidhtNow" select="/pdf2xml/page[@number='1']/@width"></xsl:variable>
    <xsl:variable name="HandleAffe"><xsl:text>#79824#</xsl:text></xsl:variable>
    
    
    
    <!-- variable stop -->  
    
    <!-- root to build body html -->
    <xsl:template match="/">
    <xsl:text disable-output-escaping="yes">
        
    </xsl:text>
        <xsl:comment> parse xml output from pdftohtml xml variant </xsl:comment>
        <html>
            <xsl:call-template name="InfoDoc" />
            <head>
                <meta http-equiv="Content-Type" content="text/html;charset=utf-8"/>
                <meta name="ConvertTime" content="$Convert_Time"/>
                <meta name="FileName" content="$Filename"/>
                <xsl:call-template name="Metaobject" />
                <title>
                    <xsl:value-of select="$Title_Document"/>
                </title>
                <xsl:call-template name="CssRender"/> 
            </head>
            <body>
                <div id="WrapperPage">
                    <xsl:if test="$debugcss = '0'">
                        <xsl:apply-templates select="/pdf2xml"/> 
                    </xsl:if>
                </div>
                <div id="FooterInfo">
                    <!-- document owner name date ecc...  -->
                </div>
            </body>
        </html>
    </xsl:template>
    
    <xsl:template match="/pdf2xml">
        <!-- center in wrapper margin auto! -->
        <div id="PageGroup">
            <div class="PageSpace">
                <!-- pagespace --> 
                <xsl:comment> pagespace </xsl:comment>
            </div>
            <xsl:apply-templates/>
        </div>
    </xsl:template>
    
    <xsl:template match="page">
        <xsl:variable name="nextnodename">
            <xsl:value-of select="name(*[1])"/>
        </xsl:variable>
        <xsl:if test="$nextnodename !='' ">
            <div>
                <xsl:attribute name="id">
                    <xsl:text>PageNr_</xsl:text>
                    <xsl:value-of select="@number"/>
                </xsl:attribute>
                <xsl:attribute name="class">
                    <xsl:text>FlyPage</xsl:text>
                </xsl:attribute>
                <xsl:attribute name="style">
                    <xsl:text>height:</xsl:text>
                    <xsl:value-of select="@height"/>
                    <xsl:text>px</xsl:text>
                    <xsl:text>;</xsl:text>
                    <xsl:text>width:</xsl:text>
                    <xsl:value-of select="@width"/>
                    <xsl:text>px</xsl:text>
                    <xsl:text>;</xsl:text>
                </xsl:attribute>
                <xsl:apply-templates/>
            </div>
            <div class="PageSpace">
                <!-- pagespace --> 
                <xsl:comment> pagespace </xsl:comment>
            </div>
        </xsl:if>
    </xsl:template>
    
    
    <xsl:template match="text">
        <p>
            <xsl:attribute name="class">
                <xsl:value-of select="concat($PrependTagClass,@font)"/>
            </xsl:attribute>
            <xsl:attribute name="style">
                <xsl:text>position:absolute;height:</xsl:text>
                <xsl:value-of select="@height"/>
                <xsl:text>px</xsl:text>
                <xsl:text>;</xsl:text>
                <xsl:text>width:</xsl:text>
                <xsl:value-of select="@width"/>
                <xsl:text>px</xsl:text>
                <xsl:text>;</xsl:text>
                <xsl:text>top:</xsl:text>
                <xsl:value-of select="@top"/>
                <xsl:text>px</xsl:text>
                <xsl:text>;</xsl:text>
                <xsl:text>left:</xsl:text>
                <xsl:value-of select="@left"/>
                <xsl:text>px</xsl:text>
                <xsl:text>;</xsl:text>
            </xsl:attribute>
            <xsl:apply-templates/>
        </p>
    </xsl:template>
    
    
    
    
    <xsl:template match="image">
        
        <xsl:variable name="grepimage">
            <xsl:value-of select="concat($PrependImage,@src)"/>
        </xsl:variable>
        
        <div>
            <xsl:attribute name="style">
                <xsl:text>position:absolute;height:</xsl:text>
                <xsl:value-of select="@height"/>
                <xsl:text>px</xsl:text>
                <xsl:text>;</xsl:text>
                <xsl:text>width:</xsl:text>
                <xsl:value-of select="@width"/>
                <xsl:text>px</xsl:text>
                <xsl:text>;</xsl:text>
                <xsl:text>top:</xsl:text>
                <xsl:value-of select="@top"/>
                <xsl:text>px</xsl:text>
                <xsl:text>;</xsl:text>
                <xsl:text>left:</xsl:text>
                <xsl:value-of select="@left"/>
                <xsl:text>px</xsl:text>
                <xsl:text>;</xsl:text>
            </xsl:attribute>
            <img>
                <xsl:attribute name="data">
                    <xsl:text>embedded=</xsl:text>
                    <xsl:value-of select="@src"/>
                </xsl:attribute>
                <xsl:attribute name="src">
                    <xsl:value-of select="$grepimage"/>
                </xsl:attribute>
                <xsl:attribute name="height">
                    <xsl:value-of select="@height"/>
                </xsl:attribute>
                <xsl:attribute name="width">
                    <xsl:value-of select="@width"/>
                </xsl:attribute>
            </img>
        </div>
    </xsl:template>
    
    
    <xsl:template match="a">
        <a target="_blank"  href="{@href}">
            <xsl:apply-templates/>
        </a>
    </xsl:template>
    
    <xsl:template match="b">
        <span class="Bold">
            <xsl:apply-templates/>
        </span>
    </xsl:template>
    
    <xsl:template match="u">
        <span class="Underline">
            <xsl:apply-templates/>
        </span>
    </xsl:template>
    
    <xsl:template match="i">
        <span class="Italic">
            <xsl:apply-templates/>
        </span>
    </xsl:template>
    
    <xsl:template match="text()">
        <xsl:value-of select="translate(.,'&#x20;&#x9;&#xD;&#xA;', ' ')"/>
    </xsl:template>
    
    <xsl:template match="fontspec">
    </xsl:template>
    
    
    <xsl:template name="CssRender">
        <!--   border-left:1px solid red; --> 
        <style type="text/css">
            <xsl:comment>
                * { margin:0;padding:0;}
                body { background-color:#A0A0A0; }
                p { display:block; }
                a:active,a:focus,a:visited,a:link,a { color:blue; text-decoration:underline; }
                a:hover { color:red; text-decoration:underline; }
                div.FlyPage {  position:relative; }
                p,div,span { white-space:nowrap; }
                .Bold { font-weight:bold; }
                .Italic { font-style:italic; }
                .Underline { text-decoration:underline; }
                div.PageSpace { width:100%;height:10px;background-color:#A0A0A0;}
                #WrapperPage { margin-left:auto; margin-right:auto; background-color:#fff;width:<xsl:value-of select="$PageWidhtNow"/>px; }
    
                <xsl:for-each select="//fontspec">
                    <xsl:text>.</xsl:text>
                    <xsl:value-of select="concat($PrependTagClass,@id)"/>
                    <xsl:text> {</xsl:text> 
                    <xsl:text>font-size:</xsl:text>
                    <xsl:value-of select="@size"/>
                    <xsl:text>px;</xsl:text> 
                    <xsl:text>font-family:</xsl:text>
                    <xsl:value-of select="@family"/>
                    <xsl:text>;</xsl:text> 
                    <xsl:text>color:</xsl:text>
                    <xsl:value-of select="@color"/>
                    <xsl:text>;white-space:nowrap;</xsl:text> 
                    <xsl:text> }
                    </xsl:text>
                </xsl:for-each>
    
                .xflip {
                -moz-transform: scaleX(-1);
                -webkit-transform: scaleX(-1);
                -o-transform: scaleX(-1);
                transform: scaleX(-1);
                filter: fliph;
                }
                .yflip {
                -moz-transform: scaleY(-1);
                -webkit-transform: scaleY(-1);
                -o-transform: scaleY(-1);
                transform: scaleY(-1);
                filter: flipv;
                }
                .xyflip {
                -moz-transform: scaleX(-1) scaleY(-1);
                -webkit-transform: scaleX(-1) scaleY(-1);
                -o-transform: scaleX(-1) scaleY(-1);
                transform: scaleX(-1) scaleY(-1);
                filter: fliph + flipv;
                }
</xsl:comment>



        </style>
        
        
    </xsl:template>
    
    <xsl:template name="InfoDoc">
    </xsl:template>
    
    <xsl:template name="FooterInfo">
    </xsl:template>
    
    <xsl:template name="Metaobject">
    </xsl:template>
    





    <!--   xsltproc pdf2html.xsl sample.xml  -->
</xsl:stylesheet>
