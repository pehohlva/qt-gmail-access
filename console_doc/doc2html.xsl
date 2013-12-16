<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" 
                xmlns:fo="http://www.w3.org/1999/XSL/Format" 
                xmlns:cms="http://www.freeroad.ch/2013/CMSFormat" 
                xmlns:dc="http://purl.org/dc/elements/1.1/" 
                xmlns:fn="http://www.w3.org/2005/xpath-functions"
                xmlns:xs="http://www.w3.org/2001/XMLSchema" 
                xmlns:fox="http://xmlgraphics.apache.org/fop/extensions"
                xmlns:int="http://catcode.com/odf_to_xhtml/internal" 
                xmlns:math="http://www.w3.org/1998/Math/MathML" 
                xmlns:svg="urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0" 
                xmlns:xforms="http://www.w3.org/2002/xforms" 
                xmlns:xlink="http://www.w3.org/1999/xlink" 
                exclude-result-prefixes="fo cms"
                version="1.0">
    <xsl:output method="xml"
                indent="yes"
                omit-xml-declaration="yes"
                doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN"
                doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd"
                encoding="UTF-8" />

    <xsl:variable name="ConvertTime" select="//fo:head/fo:meta/fo:make_time"/>
    <xsl:variable name="DocumentName" select="//fo:head/fo:meta/fo:filename"/>
    <!-- filename doc2html.xsl  doc no image!!   -->
    <xsl:variable name="lineBreak">
        <xsl:text>
        </xsl:text>
    </xsl:variable>
    <xsl:variable name="tabtab" xml:space="preserve"><xsl:text>   </xsl:text></xsl:variable>
    <xsl:variable name="spaces" xml:space="preserve"/>
    <xsl:variable name="debugmy" select="'0'"></xsl:variable>
    <xsl:variable name="onlymastercss" select="'0'"></xsl:variable>
    <xsl:variable name="debugcss" select="'0'"></xsl:variable>
    
    <!-- root to build body html -->
    <xsl:template match="/">
        <html>
            <head>
                <meta http-equiv="Content-Type" content="text/html;charset=utf-8"/>
                <meta name="Generator" content="XSLT gen on {$ConvertTime}" />
                <xsl:call-template name="meta" />
                <title>
                    <xsl:value-of select="$DocumentName"/><xsl:text>   </xsl:text> <xsl:value-of select="//fo:head/fo:meta/fo:title"/>
                    <xsl:text> - </xsl:text>
                    <xsl:value-of select="$ConvertTime"/>
                </title> 
                <xsl:call-template name="cssrender"/> 
            </head>
            <body>
                <div id="WrapperPage">
                    <div id="Page">
                        <xsl:if test="$debugcss = '0'">
                            <xsl:apply-templates select="/fo:root/fo:office/fo:page"/>
                        </xsl:if>
                    </div>
                </div>
                <div id="FooterInfo">
                    <xsl:call-template name="info_debug"></xsl:call-template>
                </div>
            </body>
        </html>
    </xsl:template>
   
    <xsl:template match="fo:span">
        <span id="{@id}" class="{translate(@class,'.','_')}" style="{@style}">
            <xsl:apply-templates/>
        </span>
    </xsl:template>
    
    <xsl:template match="fo:para">
        <p data="p36" id="{@id}" class="{translate(@class,'.','_')}" style="{@style}">
            <xsl:apply-templates/>
        </p>
    </xsl:template>
    
    <xsl:template match="fo:link">
        <a target="_blank"  href="{@href}">
            <xsl:apply-templates/>
        </a>
    </xsl:template>

     <xsl:template match="fo:s">
        <img  data="R85" class="space"  width="5px" height="5px" src="data:image/gif;base64,R0lGODlhAQABAAAAACH5BAEKAAEALAAAAAABAAEAAAICTAEAOw==" alt="text:space" />
    </xsl:template> 

     <xsl:template match="fo:tab">
        <img  data="R88" class="tab"  width="33px" height="10px" src="data:image/gif;base64,R0lGODlhAQABAAAAACH5BAEKAAEALAAAAAABAAEAAAICTAEAOw==" alt="text:tab" />
    </xsl:template> 
    
    <xsl:template match="fo:end">
        <xsl:comment>para end.</xsl:comment>
    </xsl:template>
    
    <xsl:template match="fo:emptyp">
        <!-- null paragraph  -->
        <img data="R98" class="nullpara"  width="11px" height="11px" src="data:image/gif;base64,R0lGODlhAQABAAAAACH5BAEKAAEALAAAAAABAAEAAAICTAEAOw==" alt="text:nulpara" />
    </xsl:template>
   
    <xsl:template match="fo:body_section">
        <!-- end ducument if run is bodyEnd -->
        <xsl:apply-templates/>
    </xsl:template>
     
    <xsl:template match="fo:draw_object">
    </xsl:template>

    <xsl:template match="fo:footer">
        <xsl:apply-templates/>
    </xsl:template>
    
    <xsl:template match="fo:page">
        <xsl:apply-templates/>
    </xsl:template>

    <xsl:template match="fo:header_virtual">
    </xsl:template>
    
    <xsl:template match="fo:header_full">
    </xsl:template>
    
    <xsl:template match="fo:root">
    </xsl:template>
    
    
    <xsl:template match="fo:css">
        <xsl:text>.</xsl:text><xsl:value-of select="@class"/><xsl:text> { font-style:</xsl:text><xsl:value-of select="@font"/> <xsl:text>; }</xsl:text>
    </xsl:template>
    
     <xsl:template match="fo:pagemaster">
    </xsl:template>
    
    <xsl:template match="fo:office">
        <xsl:apply-templates/>
    </xsl:template>
    
    <xsl:template name="meta">
        <xsl:if test="$debugmy = '1'">
            <meta data="converter-from" content="Busy"/>
        </xsl:if>
    </xsl:template>

    <xsl:template name="cssrender">
        <xsl:value-of select="$lineBreak"/>
        <xsl:comment></xsl:comment>
        <style type="text/css">
            <xsl:value-of select="$lineBreak"/>
            <!-- margin:1.7cm; #Page ( width:20cm;min-height:22cm;margin:1cm; background-color:red; )-->
            <xsl:text> 
                * {margin:0;padding:0;}
                body { background-color:#A0A0A0; }
                table { width:100%; }
                div { position:static; }
                #WrapperPage { padding:1.4cm; width:22cm;background-color:#fff;margin-left:auto;margin-right:auto; }
            </xsl:text>
            <xsl:value-of select="$lineBreak"/>
            <xsl:if test="$onlymastercss = '0'">
                <xsl:apply-templates select="/fo:root/fo:head/fo:style"/>
            </xsl:if>
            <xsl:value-of select="$lineBreak"/>
        </style> 
         <xsl:value-of select="$lineBreak"/>
          <xsl:comment>
                This file handle doc binary xml version
                New Revision from Peter Hohl 2013 pehohlva@gmail.com
                Document generator time: <xsl:value-of select="$ConvertTime"/>
                - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
                Apache 2.0 
                Licensed under the Apache License, Version 2.0 (the "License");
                you may not use this file except in compliance with the License.
                You may obtain a copy of the License at

                http://www.apache.org/licenses/LICENSE-2.0

                Unless required by applicable law or agreed to in writing, software
                distributed under the License is distributed on an "AS IS" BASIS,
                WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
                See the License for the specific language governing permissions and
                limitations under the License.
            </xsl:comment> 
           <xsl:value-of select="$lineBreak"/> 
    </xsl:template>
    
    <xsl:template name="info_debug">
        <xsl:value-of select="$lineBreak"/><xsl:value-of select="$lineBreak"/>
        <xsl:for-each select="//fo:head/fo:meta/*">
            <xsl:comment>
                <xsl:text>Name:</xsl:text>  
                <xsl:value-of select="concat(name(), ' : ', .)"/>
            </xsl:comment>
            <xsl:value-of select="$lineBreak"/>
        </xsl:for-each>
    </xsl:template>
    
    
    
    

</xsl:stylesheet>
