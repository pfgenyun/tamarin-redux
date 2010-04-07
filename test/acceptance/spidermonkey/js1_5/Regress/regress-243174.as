/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is JavaScript Engine testing utilities.
 *
 * The Initial Developer of the Original Code is
 * Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s): Brendan Eich <brendan@mozilla.org>
 *                 Bob Clary <bob@bclary.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

startTest();

var gTestfile = 'regress-243174.js';
//-----------------------------------------------------------------------------
var BUGNUMBER = 243174;
var summary = 'Don\'t Crash on Regular Expression';
var actual = 'Crash';
var expect = 'No Crash';


printBugNumber(BUGNUMBER);
printStatus (summary);


var result = "-+16-+59-+66-+67-+80-+82-+143-+170-+176-+189-+308-+363-+364-+365-+377-+393-+404-+405-+419-+430-+641-+732-+754-+783-+786-+972-+977-+980-+982-+1010-+1011-+1027-+1028-+1039-+1040-+1074-+1084-+1086-+1098-+1267-+1296-+1305-+1367-+1371-+1379-+1480-+1481-+1482-+1484-+1510-+1526-+1565-+1568-+1574-+1577-+1604-+1632-+1638-+1643-+1657-+1708-+1722-+1941-+1948-+1955-+1965-+1966-+2027-+2039-+2040-+2041-+2048-+2054-+2059-+2090-+2091-+2092-+2105-+2118-+".match(/\\+(3|4|7|21|47|49|53|54|56|57|58|59|60|61|62|64|67|69|72|73|74|76|78|80|84|91|95|96|99|118|120|141|142|145|147|148|149|151|152|160|164|169|170|171|173|174|175|176|181|183|185|186|188|189|190|191|193|200|201|202|204|205|207|208|209|211|214|216|221|223|226|229|230|231|233|237|239|249|250|252|255|258|260|261|267|269|270|278|280|281|290|291|293|294|295|296|297|298|299|300|301|302|303|306|307|308|309|311|313|317|319|321|322|328|329|338|342|343|345|347|349|352|359|360|364|366|367|368|370|373|376|377|378|379|380|381|384|385|386|387|388|389|390|393|394|396|397|398|399|400|402|403|416|418|419|420|423|424|425|428|429|430|432|440|442|444|445|446|448|449|629|643|646|647|649|652|658|668|680|681|682|683|684|703|706|720|722|731|733|736|737|738|741|744|745|749|752|753|754|755|763|786|803|806|807|808|812|829|831|843|844|845|846|847|848|849|851|854|855|856|858|859|860|861|863|866|867|868|869|870|871|875|876|877|878|879|881|882|883|884|885|886|888|889|890|891|892|893|894|895|896|897|898|900|901|903|904|906|908|910|911|912|913|914|915|916|918|919|921|970|971|972|973|980|986|987|988|991|998|1009|1011|1015|1016|1031|1037|1038|1039|1040|1045|1046|1051|1052|1053|1054|1057|1058|1060|1064|1069|1070|1071|1074|1075|1085|1089|1090|1091|1093|1094|1095|1096|1097|1101|1103|1107|1109|1110|1112|1115|1116|1117|1171|1172|1175|1183|1184|1233|1289|1296|1300|1307|1315|1317|1327|1367|1374|1384|1392|1393|1408|1409|1412|1428|1479|1480|1481|1482|1483|1484|1485|1486|1487|1488|1490|1491|1492|1493|1497|1510|1522|1524|1565|1566|1567|1568|1573|1574|1576|1582|1584|1586|1588|1591|1592|1593|1596|1599|1600|1604|1606|1615|1616|1617|1621|1625|1631|1632|1633|1636|1640|1643|1644|1645|1646|1648|1650|1652|1655|1656|1657|1658|1660|1661|1663|1669|1670|1671|1672|1673|1675|1676|1677|1679|1680|1683|1684|1685|1686|1687|1688|1689|1695|1697|1702|1703|1704|1705|1706|1711|1712|1713|1714|1716|1722|1725|1726|1731|1738|1744|1747|1748|1749|1750|1753|1757|1761|1762|1763|1764|1765|1766|1767|1769|1771|1772|1773|1774|1775|1776|1777|1778|1779|1780|1781|1782|1783|1784|1785|1786|1788|1789|1790|1791|1792|1793|1794|1796|1797|1798|1799|1801|1802|1803|1804|1805|1806|1807|1808|1809|1810|1811|1812|1815|1816|1817|1818|1821|1822|1823|1824|1825|1827|1828|1831|1835|1840|1844|1845|1849|1850|1852|1853|1854|1855|1856|1857|1858|1859|1860|1862|1866|1867|1874|1885|1886|1887|1890|1894|1897|1898|1903|1912|1913|1917|1923|1933|1940|1941|1944|1945|1946|1947|1948|1949|1950|1963|1964|1965|1967|1971|1972|1973|1974|1978|1983|1988|1990|1991|2001|2003|2013|2015|2020|2025|2026|2027|2029|2034|2039|2040|2041|2047|2048|2049|2050|2053|2054|2055|2057|2058|2059|2060|2061|2064|2067|2070|2073|2076|2079|2082|2085|2088|2090|2092|2093|2094|2095|2096|2098|2099|2100|2101|2102|2103|2105|2114|2119|2121|2122|2124|2128|2131|2132|21|170|177|190|191|291|982|1038|1655|1978|2090|2133|1983|783|1582|2102|6|14|53|65|66|67|68|72|85|88|95|96|97|121|126|145|148|154|160|184|188|219|220|258|267|277|289|292|295|297|304|317|318|322|332|342|343|353|354|367|373|378|381|384|398|402|418|419|425|428|438|643|662|665|673|675|705|706|803|876|973|988|1013|1015|1020|1047|1091|1171|1184|1317|1400|1401|1486|1572|1590|1591|1593|1600|1621|1632|1633|1635|1636|1638|1640|1648|1657|1958|1966|1969|1973|1983|2048|2061|2064|2067|2070|2073|2076|2079|2082|2085|2088|2091|2126|2127|2128|1063|986|16|59|66|67|80|82|143|170|176|189|308|363|364|365|377|393|404|405|419|430|641|732|754|783|786|972|977|980|982|1010|1011|1027|1028|1039|1040|1074|1084|1086|1098|1267|1296|1305|1367|1371|1379|1480|1481|1482|1484|1510|1526|1565|1568|1574|1577|1604|1632|1638|1643|1657|1708|1722|1941|1948|1955|1965|1966|2027|2039|2040|2041|2048|2054|2059|2090|2091|2092|2105|2118|1300|971|2047|2050|986|1632|2049|1184|2047)-/g);

actual = 'No Crash';

AddTestCase(summary, expect, actual);

test();
