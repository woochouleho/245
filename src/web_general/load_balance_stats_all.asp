<% SendWebHeadStr();%>
<title><% multilang(LANG_LOAD_BALANCE_STATS); %></title>
<script>
var ld_mode = 0;

function generate_random_str()
{
	var d = new Date();
	var str=d.getFullYear()+"."+(d.getMonth()+1)+"."+d.getDate()+"."+d.getHours()+"."+d.getMinutes()+"."+d.getSeconds();
	return str;
}

function on_init_iframe(){
	modeSelection(ld_mode);
}	

function modeSelection(mode){
	var iframe_ele = document.getElementById("iframe_id");
	if(mode == 0){
		iframe_ele.src ="./load_balance_stats_table.asp?t="+generate_random_str();
	}else {			
		iframe_ele.src ="./load_balance_stats_chart.asp?t="+generate_random_str();	
	}
}
</script>
</head>

<body onload="on_init_iframe();">

<div class="intro_main ">
<%checkWrite("multi_phy_wan_formal_start"); %>
<p class="intro_title"><% multilang(LANG_LOAD_BALANCE_STATS); %></p>
<p class="intro_content"><% multilang(LANG_LOAD_BALANCE_STATS_INFO); %></p>
<%checkWrite("multi_phy_wan_formal_end"); %>

<%checkWrite("multi_phy_wan_demo_start"); %>
<p class="intro_title" style="font-size:28px; text-align:center">Multiple WAN Status</p>
<%checkWrite("multi_phy_wan_demo_end"); %>
</div>

<br>
<form action=/boaform/formLdStatsMode method=POST name="formLdStatsMode">
<div>
<tr>
	<th width="30%">Statistics:   &nbsp;&nbsp;</th>
	<td>
		<input type="radio" value="0" name="mode" checked onClick="modeSelection(0)">Table  &nbsp;&nbsp;
		<input type="radio" value="1" name="mode" onClick="modeSelection(1)">Chart
	</td>
</tr>
</div>

<br>
<br>

<table style="width:100%">
<tr>		
	<iframe id="iframe_id" style="width:100%; height:500px; padding-top:10px">
	</iframe>
</tr>

</table>
</form>
</body>

</html>
