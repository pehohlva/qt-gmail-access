<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" 
                xmlns:dc="http://purl.org/dc/elements/1.1/" 
                xmlns:fox="http://xmlgraphics.apache.org/fop/extensions"
                xmlns:fo="http://www.w3.org/1999/XSL/Format" 
                xmlns:int="http://catcode.com/odf_to_xhtml/internal" 
                xmlns:math="http://www.w3.org/1998/Math/MathML" 
                xmlns:svg="urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0" 
                xmlns:xforms="http://www.w3.org/2002/xforms" 
                xmlns:xlink="http://www.w3.org/1999/xlink" 
                xmlns:cms="http://www.freeroad.ch/2013/CMSFormat" 
                version="2.0">
    
    <xsl:output method="xml"
                indent="yes"
                omit-xml-declaration="yes"
                doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN"
                doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd"
                encoding="UTF-8" />


    <xsl:variable name="lineBreak">
        <xsl:text>
        </xsl:text>
    </xsl:variable>

    <xsl:variable name="tabulator">
        <xsl:text>      </xsl:text>
    </xsl:variable>
    <xsl:variable name="tabtab" xml:space="preserve"><xsl:text>   </xsl:text></xsl:variable>
    <xsl:variable name="spaces" xml:space="preserve"/>
    <xsl:variable name="debugmy" select="'0'"></xsl:variable>
    <xsl:variable name="onlymastercss" select="'0'"></xsl:variable>
    <xsl:variable name="debugcss" select="'0'"></xsl:variable>
    <xsl:variable name="ConvertTime" select="/fo:root/fo:document-meta/@converted-h"></xsl:variable>
    
   
    
    <!-- root to build body html -->
    <xsl:template match="/">
        <html>
            <xsl:call-template name="infodoc" />
            <head>
                <meta http-equiv="Content-Type" content="text/html;charset=utf-8"/>
                <xsl:comment> parse odt meta.xml styles.xml content.xml - meta.xml  </xsl:comment>
                <xsl:comment> Metadata ends next css  </xsl:comment>
                <xsl:call-template name="meta" />
                <title> documento </title>
                
                <xsl:call-template name="cssrender"/> 
                
            </head>
            <body>
                <div id="WrapperPage">
                    <xsl:if test="$debugcss = '0'">
                        <xsl:apply-templates select="/fo:root/fo:document-content/fo:body/fo:text"/> 
                    </xsl:if>
                </div>
                <div id="FooterInfo">
                    <xsl:call-template name="timegenerator"></xsl:call-template>
                </div>
            </body>
        </html>
    </xsl:template>
    
    <xsl:template match="/fo:root">
    </xsl:template>
    
    <xsl:template name="infodoc">
        <xsl:if test="$debugcss = '0'">
            <xsl:comment>
                <xsl:value-of select="//fo:page-layout/fo:page-layout-properties/@page-width"/>
                This file handle a union from odt xml
                "meta.xml" , "styles.xml" , "content.xml" and object if exist
                why? xslt2 not support xsl:include file by QXmlQuery
                and Xslt1 gnome lib is old and not having new function.
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
        </xsl:if>
    </xsl:template>
    
    <xsl:template name="meta">
        <xsl:if test="$debugmy = '1'">
            <meta data="converter-from" content="{/fo:root/fo:document-meta/@converted-h}"/>
        </xsl:if>
    </xsl:template>


    <!--  XXXXXXXXXXX style section paragraph XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX  -->
    <!--  XXXXXXXXXXX style section paragraph XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX  -->
    <!--  XXXXXXXXXXX style section paragraph XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX  --> 

    
    <xsl:template match="fo:p">
        <xsl:variable name="nextnodename">
            <xsl:value-of select="name(*[1])"/>
        </xsl:variable>
        <xsl:if test="$debugmy = '1'">
            <p>Prossimo nodo:(<xsl:value-of select="$nextnodename"/>)</p>
        </xsl:if>
        <xsl:variable name="strlesize" select="string-length(@style-name)"/>
    
        <xsl:choose>
            <xsl:when test="$strlesize &gt;= 2">
                <p data="RL115" class="{translate(@style-name,'.','_')}">
                    <xsl:apply-templates/>
                    <xsl:if test="count(node())=0">
                        <br/>
                    </xsl:if>
                </p>
            </xsl:when>
            <xsl:otherwise>
                <p data="RL123">
                    <xsl:apply-templates/>
                    <xsl:if test="count(node())=0">
                        <br/>
                    </xsl:if> 
                </p>
            </xsl:otherwise>
        </xsl:choose>
    </xsl:template>    
      
    <xsl:template match="fo:span">
        <span data="RL134" class="{translate(@style-name,'.','_')}">
            <xsl:apply-templates/>
        </span>
    </xsl:template>

    <xsl:template match="fo:tab">
        <!-- put image pixel transparent --> 
        <img  data="render93" class="tab"  width="75px" height="1px" src="data:image/gif;base64,R0lGODlhAQABAAAAACH5BAEKAAEALAAAAAABAAEAAAICTAEAOw==" alt="text:tab" />
    </xsl:template> 
      
      
    <xsl:template match="line-break">
        <br />
    </xsl:template>
    

    <xsl:template match="fo:a">
        <a target="_blank"  href="{@xlink:href}">
            <xsl:apply-templates/>
        </a>
    </xsl:template>
    
    
    <xsl:template match="fo:s">
        <xsl:choose>
            <xsl:when test="@c">
                <xsl:call-template name="insert-spaces">
                    <xsl:with-param name="n" select="@c"/>
                </xsl:call-template>
            </xsl:when>
            <xsl:otherwise>
                <img width="5px" height="10px" data="line326" src="data:image/gif;base64,R0lGODlhAQABAAAAACH5BAEKAAEALAAAAAABAAEAAAICTAEAOw==" alt="fo:s" />
            </xsl:otherwise>
        </xsl:choose>
    </xsl:template>
    
    <xsl:template name="insert-spaces">
        <xsl:param name="n"/>
        <xsl:choose>
            <xsl:when test="$n &lt;= 30">
                <xsl:value-of select="substring($spaces, 1, $n)"/>
            </xsl:when>
            <xsl:otherwise>
                <xsl:value-of select="$spaces"/>
                <xsl:call-template name="insert-spaces">
                    <xsl:with-param name="n">
                        <xsl:value-of select="$n - 30"/>
                    </xsl:with-param>
                </xsl:call-template>
            </xsl:otherwise>
        </xsl:choose>
    </xsl:template>
    
    
    <xsl:template match="fo:h">
        <!-- Heading levels go only to 6 in XHTML -->
        <xsl:variable name="level">
            <xsl:choose>
                <xsl:when test="@outline-level &gt; 6">6</xsl:when>
                <xsl:otherwise>
                    <xsl:value-of select="@outline-level"/>
                </xsl:otherwise>
            </xsl:choose>
        </xsl:variable>
        <xsl:element name="{concat('h', $level)}">
            <xsl:attribute name="class">
                <xsl:value-of
                    select="translate(@style-name,'.','_')"/>
            </xsl:attribute>
            <xsl:apply-templates/>
        </xsl:element>
    </xsl:template>
    
    
    <!--  XXXXXXXXXXX style section to table XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX  -->
    <!--  XXXXXXXXXXX style section to table XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX  -->
    <!--  XXXXXXXXXXX style section to table XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX  -->
    
    
    <xsl:template match="fo:table">
        <xsl:variable name="nametable">
            <xsl:value-of select="@style-name"/>
        </xsl:variable>
        <xsl:comment>process table cell con valori n <xsl:value-of select="@style-name"/></xsl:comment>
        <table class="{$nametable}">
            <colgroup>
                <xsl:apply-templates select="fo:table-column"/>
            </colgroup>
            <xsl:if test="fo:table-header-rows/fo:table-row">
                <thead>
                    <xsl:apply-templates select="fo:table-header-rows/fo:table-row"/>
                </thead>
            </xsl:if>
            <tbody>
                <xsl:apply-templates select="fo:table-row"/>
            </tbody>
        </table>
        
    </xsl:template> 
    
    <xsl:template match="fo:table-column">
        <col>
            <xsl:if test="@number-columns-repeated">
                <xsl:attribute name="span">
                    <xsl:value-of select="@number-columns-repeated"/>
                </xsl:attribute>
            </xsl:if>
            <xsl:if test="@style-name">
                <xsl:attribute name="class">
                    <xsl:value-of select="translate(@style-name,'.','_')"/>
                </xsl:attribute>
            </xsl:if>
        </col>
    </xsl:template>
    
    <xsl:template match="fo:table-row">
        <tr>
            <xsl:apply-templates select="fo:table-cell"/>
        </tr>
    </xsl:template> 
    
    <xsl:template match="fo:table-cell">
        <xsl:variable name="n">
            <xsl:choose>
                <xsl:when test="@number-columns-repeated != 0">
                    <xsl:value-of select="@number-columns-repeated"/>
                </xsl:when>
                <xsl:otherwise>1</xsl:otherwise>
            </xsl:choose>
        </xsl:variable>
        <xsl:call-template name="process-table-cell">
            <xsl:with-param name="n" select="$n"/>
        </xsl:call-template>
    </xsl:template>



    <!--    
        <xsl:comment>process table cell con valori n <xsl:value-of select="$n"/></xsl:comment>
        
    -->

    <xsl:template name="process-table-cell">
        <xsl:param name="n"/>
        <xsl:if test="$n != 0">
            <td>
                <xsl:if test="@style-name">
                    <xsl:attribute name="class">
                        <xsl:value-of select="translate(@style-name,
					'.','_')"/>
                    </xsl:attribute>
                </xsl:if>
                <xsl:if test="@number-columns-spanned">
                    <xsl:attribute name="colspan">
                        <xsl:value-of select="@number-columns-spanned"/>
                    </xsl:attribute>
                </xsl:if>
                <xsl:if test="@number-rows-spanned">
                    <xsl:attribute name="rowspan">
                        <xsl:value-of select="@number-rows-spanned"/>
                    </xsl:attribute>
                </xsl:if>
                <xsl:apply-templates/>
            </td>
            <xsl:call-template name="process-table-cell">
                <xsl:with-param name="n" select="$n - 1"/>
            </xsl:call-template>
        </xsl:if>
    </xsl:template>
    
    <!--  XXXXXXXXXXX style section to css XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX  -->
    <!--  XXXXXXXXXXX style section to css XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX  -->
    <!--  XXXXXXXXXXX style section to css XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX  -->
      
    <!-- <xsl:text>margin-left:</xsl:text> <xsl:value-of select="//fo:page-layout/fo:page-layout-properties/@margin-left"/><xsl:text>;</xsl:text>
    <xsl:text>margin-right:</xsl:text> <xsl:value-of select="//fo:page-layout/fo:page-layout-properties/@margin-right"/><xsl:text>;</xsl:text>
    --> 
    <xsl:template name="cssrender">
        <xsl:value-of select="$lineBreak"/>
        <xsl:comment></xsl:comment>
        <style type="text/css">
            <xsl:value-of select="$lineBreak"/>
            <xsl:text> #WrapperPage 
                { position:relative;width:</xsl:text> 
            <xsl:value-of select="//fo:page-layout/fo:page-layout-properties/@page-width"/>
            <xsl:text>;</xsl:text>
            <xsl:text>background-color:</xsl:text> 
            <xsl:value-of select="//fo:page-layout/fo:page-layout-properties/@background-color"/>
            <xsl:text>;</xsl:text>
            <xsl:text>margin-top:</xsl:text> 
            <xsl:value-of select="//fo:page-layout/fo:page-layout-properties/@margin-top"/>
            <xsl:text>;</xsl:text>
            <xsl:text>margin-bottom:</xsl:text> 
            <xsl:value-of select="//fo:page-layout/fo:page-layout-properties/@margin-bottom"/>
            <xsl:text>;</xsl:text>
            <xsl:text>margin-left:auto;</xsl:text>
            <xsl:text>margin-right:auto;</xsl:text>
            <xsl:text>}</xsl:text>
            <xsl:value-of select="$lineBreak"/>
            <xsl:text>/* Placeholder base style */</xsl:text>
            .GTZHHGJKGBTRSDER36HL34GT56{}
            .GTZHHGJKGB264239462GSHFAS8{}
            <xsl:text>/* Placeholder base style */</xsl:text>
            <xsl:value-of select="$lineBreak"/>
            <xsl:apply-templates select="fo:root/fo:document-styles/fo:css"/>
            <xsl:if test="$onlymastercss = '0'">
                <xsl:apply-templates select="/fo:root/fo:document-content/fo:automatic-styles"/> 
            </xsl:if>
            
        </style>
    </xsl:template>    
    
    
    <!-- all possibel style css from  fo:automatic-styles  -->
    <xsl:template match="fo:style">
        <xsl:if test="$onlymastercss = '0'">
            <xsl:call-template name="css-switch"></xsl:call-template>
        </xsl:if>
    </xsl:template>
   
    <!-- all possibel style css from down fo:css fo:default -->
    <xsl:template match="fo:default-style">
        <xsl:call-template name="css-switch"></xsl:call-template>
    </xsl:template>
    
    
    <!--    
    <xsl:text> /* nome= </xsl:text>
    <xsl:value-of select="string-length(@name)" /> 
    <xsl:text> */ </xsl:text> --> 
 
    <xsl:template name="css-switch">
        <xsl:variable name="cssname">
            <xsl:value-of select="translate(@name,'.','_')"/>
        </xsl:variable>
        
        <xsl:choose>
            <xsl:when test="@family='paragraph'">
                <xsl:call-template name="process-tag">
                    <xsl:with-param name="typehtml" select="'p'"/>
                    <xsl:with-param name="styleobject" select="$cssname"/>
                    <xsl:with-param name="writename" select="string-length(@name)"/>
                </xsl:call-template>
            </xsl:when>
            <xsl:when test="@family='text'">
                <xsl:call-template name="process-tag">
                    <xsl:with-param name="typehtml" select="'span'"/>
                    <xsl:with-param name="styleobject" select="$cssname"/>
                    <xsl:with-param name="writename" select="string-length(@name)"/>
                </xsl:call-template>
            </xsl:when>
            <xsl:when test="@family='table-row'">
                <xsl:call-template name="process-tag">
                    <xsl:with-param name="typehtml" select="'tr'"/>
                    <xsl:with-param name="styleobject" select="$cssname"/>
                    <xsl:with-param name="writename" select="string-length(@name)"/>
                </xsl:call-template>
            </xsl:when>
            <xsl:when test="@family='table-cell'">
                <xsl:call-template name="process-tag">
                    <xsl:with-param name="typehtml" select="'td'"/>
                    <xsl:with-param name="styleobject" select="$cssname"/>
                    <xsl:with-param name="writename" select="string-length(@name)"/>
                </xsl:call-template>
            </xsl:when>
            <xsl:when test="@family='table-column'">
                <xsl:call-template name="process-tag">
                    <xsl:with-param name="typehtml" select="'col'"/>
                    <xsl:with-param name="styleobject" select="$cssname"/>
                    <xsl:with-param name="writename" select="string-length(@name)"/>
                </xsl:call-template>
            </xsl:when>
            <xsl:when test="@family='table'">
                <xsl:call-template name="process-tag">
                    <xsl:with-param name="typehtml" select="'table'"/>
                    <xsl:with-param name="styleobject" select="$cssname"/>
                    <xsl:with-param name="writename" select="string-length(@name)"/>
                </xsl:call-template>
            </xsl:when>
            <xsl:when test="@family='graphic'">
                <xsl:call-template name="process-tag">
                    <xsl:with-param name="typehtml" select="'img'"/>
                    <xsl:with-param name="styleobject" select="$cssname"/>
                    <xsl:with-param name="writename" select="string-length(@name)"/>
                </xsl:call-template>
            </xsl:when>
        </xsl:choose>
    </xsl:template>
    
    
    <xsl:template match="fo:frame">
        <xsl:element name="div">
            <xsl:attribute name="class">
                <xsl:value-of select="translate(@style-name,'.','_')"/>
            </xsl:attribute>
            <xsl:attribute name="style">border: 1px solid #888;<xsl:if test="@width">width:<xsl:value-of select="@width"/>;</xsl:if>
                <xsl:if test="@height">height: <xsl:value-of select="@height"/>;</xsl:if>
            </xsl:attribute>
            
            <xsl:choose>
                <xsl:when test="fo:object/@href = fo:image/@href">
                    <xsl:variable name="imageobject">
                        <xsl:value-of select="fo:image/@href"/>
                    </xsl:variable>
                    <xsl:call-template name="externalobject">
                        <xsl:with-param name="swaproad" select="fo:object/@href"/>
                    </xsl:call-template>
                </xsl:when>
                <xsl:otherwise>
                    <xsl:apply-templates/>
                </xsl:otherwise>
            </xsl:choose>
        </xsl:element>
    </xsl:template>
    
    <xsl:template match="fo:frame/fo:object">
        <!-- do nothing --> 
    </xsl:template>
 
    <xsl:template match="fo:root/fo:obj">
        <!-- do nothing --> 
    </xsl:template>
    
    
    <!-- included on  xml external object calc charts  or elese .. fo:spreadsheet -->
    <xsl:template name="externalobject">
        <xsl:param name="swaproad"/>
        <xsl:variable name="nextsteep">
            <xsl:value-of select="substring-after(string($swaproad),'-')"/>
        </xsl:variable>
        <xsl:comment>
            <xsl:text>Draw Object nr:</xsl:text>
            <xsl:value-of select="$nextsteep"/>
        </xsl:comment>
        <xsl:if test="$nextsteep = 1">
            <xsl:apply-templates select="//fo:item/fo:obj-1/fo:document-content/fo:body/fo:spreadsheet"/>
        </xsl:if>
        <xsl:if test="$nextsteep = 2">
            <xsl:apply-templates select="//fo:item/fo:obj-2/fo:document-content/fo:body/fo:spreadsheet"/>
        </xsl:if>
        <xsl:if test="$nextsteep = 3">
            <xsl:apply-templates select="//fo:item/fo:obj-3/fo:document-content/fo:body/fo:spreadsheet"/>
        </xsl:if> 
        <xsl:if test="$nextsteep = 4">
            <xsl:apply-templates select="//fo:item/fo:obj-4/fo:document-content/fo:body/fo:spreadsheet"/>
        </xsl:if>   
        <xsl:if test="$nextsteep = 5">
            <xsl:apply-templates select="//fo:item/fo:obj-5/fo:document-content/fo:body/fo:spreadsheet"/>
        </xsl:if>
    </xsl:template>
    
    
    
 
    <xsl:template match="fo:frame/fo:image">
        <xsl:element name="img">
            <xsl:attribute name="style">width: 100%;height: 100%;<xsl:if test="not(../@anchor-type='character')">display: block;</xsl:if></xsl:attribute>
            <xsl:attribute name="alt">
                <xsl:value-of select="../@name"/>
            </xsl:attribute>
            <xsl:attribute name="title">
                <xsl:value-of select="../@name"/>
            </xsl:attribute>
            <xsl:attribute name="data">
                <xsl:value-of select="../@width"/>
                <xsl:text>|</xsl:text>
            </xsl:attribute>
            <xsl:attribute name="src">
                <xsl:value-of select="@href"/>
            </xsl:attribute>
        </xsl:element>
    </xsl:template>
 
 
    <!-- default no class --> 
    <!--  
        <xsl:text> /* process-tag </xsl:text>
        <xsl:text> </xsl:text><xsl:value-of select="$typehtml" /><xsl:text> pwd=</xsl:text><xsl:value-of select="name()" /><xsl:text>  */ </xsl:text>
    -->
    <xsl:template name="process-tag">
        <xsl:param name="typehtml"/>
        <xsl:param name="styleobject"/>
        <xsl:param name="writename"/>
        <xsl:variable name="nextstylehandler">
            <xsl:value-of select="name(*[1])"/>
        </xsl:variable>
        <xsl:value-of select="$lineBreak" />
        <xsl:choose>
            <xsl:when test="$writename = 0">
                <xsl:value-of select="$typehtml" />
            </xsl:when>
            <xsl:otherwise>
                <xsl:value-of select="$typehtml" />
                <xsl:text>.</xsl:text>
                <xsl:value-of select="$styleobject" />
            </xsl:otherwise>
        </xsl:choose>
        <xsl:text> {</xsl:text>    
        <xsl:call-template name="loopcss-items">
            <xsl:with-param name="nodeselectitem" select="$nextstylehandler"/>
        </xsl:call-template>
        <xsl:text>}</xsl:text>
    </xsl:template>
    
    
    <xsl:template name="loopcss-items">
        <xsl:param name="nodeselectitem"/>
        <!-- compatible on multiple xslt processor!!!
        fo:paragraph-properties  
        fo:text-properties
        fo:table-properties
        fo:table-row-properties
        fo:table-cell-properties
        fo:graphic-properties
        -->
        <xsl:choose>
            <xsl:when test="$nodeselectitem='fo:graphic-properties'">
                <xsl:for-each select="fo:graphic-properties/@*">
                    <xsl:value-of select="name(.)" />
                    <xsl:text>:</xsl:text>
                    <xsl:value-of select="." />
                    <xsl:text>;</xsl:text>
                </xsl:for-each>
            </xsl:when>
            <xsl:when test="$nodeselectitem='fo:text-properties'">
                <xsl:for-each select="fo:text-properties/@*">
                    <xsl:value-of select="name(.)" />
                    <xsl:text>:</xsl:text>
                    <xsl:value-of select="." />
                    <xsl:text>;</xsl:text>
                </xsl:for-each>
            </xsl:when>
            
            <!--  paragraph having sub text if having true take propriety!
            <fo:style family="paragraph" parent-style-name="Table_20_Heading" qlev="3" name="P7">
            <fo:paragraph-properties text-align="start" qlev="4" justify-single-word="false"></fo:paragraph-properties>
                 <fo:text-properties font-style="italic" font-weight-asian="normal" text-shadow="none" font-size-complex="12pt" font-name="Thorndale" font-weight="normal" color="#ffffff" qlev="4" text-underline-style="none" text-outline="false" text-line-through-style="none" font-style-asian="italic" font-weight-complex="normal" font-style-complex="italic" font-size-asian="12pt" text-overline-color="font-color" text-overline-style="none" font-size="12pt"></fo:text-properties>
            </fo:style> -->
            
            <xsl:when test="$nodeselectitem='fo:paragraph-properties'">
                <xsl:for-each select="fo:paragraph-properties/@*">
                    <xsl:value-of select="name(.)" />
                    <xsl:text>:</xsl:text>
                    <xsl:value-of select="." />
                    <xsl:text>;</xsl:text>
                </xsl:for-each>
                <xsl:for-each select="fo:text-properties/@*">
                    <xsl:value-of select="name(.)" />
                    <xsl:text>:</xsl:text>
                    <xsl:value-of select="." />
                    <xsl:text>;</xsl:text>
                </xsl:for-each>
            </xsl:when>
            <xsl:when test="$nodeselectitem='fo:table-properties'">
                <xsl:for-each select="fo:table-properties/@*">
                    <xsl:value-of select="name(.)" />
                    <xsl:text>:</xsl:text>
                    <xsl:value-of select="." />
                    <xsl:text>;</xsl:text>
                </xsl:for-each>
            </xsl:when>
            <xsl:when test="$nodeselectitem='fo:table-row-properties'">
                <xsl:for-each select="fo:table-row-properties/@*">
                    <xsl:value-of select="name(.)" />
                    <xsl:text>:</xsl:text>
                    <xsl:value-of select="." />
                    <xsl:text>;</xsl:text>
                </xsl:for-each>
            </xsl:when>
            <xsl:when test="$nodeselectitem='fo:table-cell-properties'">
                <xsl:for-each select="fo:table-cell-properties/@*">
                    <xsl:value-of select="name(.)" />
                    <xsl:text>:</xsl:text>
                    <xsl:value-of select="." />
                    <xsl:text>;</xsl:text>
                </xsl:for-each>
            </xsl:when>
        </xsl:choose>
    </xsl:template>     



    <xsl:template name="process-style-css">
    
    </xsl:template>
      
      
      <!--   <xsl:attribute name="candytype"> 
   <xsl:value-of select="name()"/> 
</xsl:attribute> @name='LHSMenu']"   --> 
<xsl:template name="timegenerator">
<xsl:for-each select="//*[@converted-h !='']">
    <xsl:comment>
<xsl:text>Time </xsl:text>
<xsl:value-of select="name()"/> <xsl:value-of select="@converted-h"/>
</xsl:comment><xsl:value-of select="$lineBreak"/>
</xsl:for-each> 

<xsl:for-each select="//fo:document-meta/fo:meta/*[@qlev =3]">
    <xsl:comment>
<xsl:text>Meta Info: </xsl:text>
<xsl:value-of select="name()"/> <xsl:text>  / </xsl:text><xsl:value-of select="."/>
</xsl:comment><xsl:value-of select="$lineBreak"/>
</xsl:for-each>


</xsl:template>

    
</xsl:stylesheet>