<% SendWebHeadStr();%>
<title><% multilang(LANG_EASYMESH_CHANNEL_SCAN); %></title>
<style>
.on {display:on}
.off {display:none}

tbody.for_top_margin::before
{
  content: '';
  display: block;
  height: 20px;
}
</style>

<SCRIPT>

</SCRIPT>
</head>

<body>
    <div class="intro_main ">
	    <p class="intro_title"><% multilang(LANG_EASYMESH_CHANNEL_SCAN); %></p>
	    <p class="intro_content"><% multilang(LANG_EASYMESH_CHANNEL_SCAN_DESC); %></p>
    </div>

<form action=/boaform/formChannelScan method=POST name="MultiAP">
	<div class="data_common data_common_notitle">
	<table>
	<tr id="channel_scan_band">
		<th>Scan Band:</th>
		<td>
		<input type="radio" id="band_5G" name="scan_band" value="5G">5G&nbsp;&nbsp;
		<input type="radio" id="band_2G" name="scan_band" value="2G">2.4G&nbsp;&nbsp;
		<input type="radio" id="band_all" name="scan_band" value="all" checked="checked">All</td>
	</tr>

	</table>
	</div>
	<div class="btn_ctl">
		<input type="submit" value="<% multilang(LANG_APPLY_CHANGES); %>" class="link_bg" name="channel_scan" onclick="">&nbsp;&nbsp;

		<input type="hidden" value="/multi_ap_channel_scan.asp" name="submit-url">
	</div>
</form>

</body>

</html>
