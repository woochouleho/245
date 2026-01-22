<% SendWebHeadStr();%>
<title><% multilang(LANG_WLAN_EASY_MESH_DEVICE_DETAILS); %></title>
<style>
.on {display:on}
.off {display:none}
</style>
<SCRIPT>

var count;
var nbTableHtmlString = "";
var staTableHtmlString = "";

function loadInfo() {
    // var urlParams = new URLSearchParams(window.location.search);
    // count = urlParams.get('count');
    var results = new RegExp('[\?&]' + 'count' + '=([^&#]*)').exec(window.location.href);
    count = decodeURI(results[1]) || 0;

	var string_json = '<% checkWrite("topology_json_string"); %>';
	var object_json = JSON.parse(string_json);

    var curr_device = get_device_json(object_json);

    nbTableHtmlString += '<tr><th width="80">MAC Address</th><th width="100">Name</th><th width="60">RSSI</th><th width="55">Connected Band</th></tr>';
    staTableHtmlString += '<tr><th width="80">MAC Address</th><th width="30">RSSI</th><th width="55">Connected Band</th><th width="60">Downlink</th><th width="60">Uplink</th></tr>';

    if (0 != curr_device["neighbor_devices"].length) {
        for (nb in curr_device["neighbor_devices"]) {
            nbTableHtmlString += '<tr><td>';
            nbTableHtmlString += curr_device["neighbor_devices"][nb].neighbor_mac;
            nbTableHtmlString += '</td><td>';
            nbTableHtmlString += curr_device["neighbor_devices"][nb].neighbor_name;
            nbTableHtmlString += '</td><td>';
            nbTableHtmlString += curr_device["neighbor_devices"][nb].neighbor_rssi;
            nbTableHtmlString += '</td><td>';
            nbTableHtmlString += curr_device["neighbor_devices"][nb].neighbor_band;
            nbTableHtmlString += '</td></tr>';

        }
    }

    if (0 != curr_device["station_info"].length) {
        for (nb in curr_device["station_info"]) {
            staTableHtmlString += '<tr><td>';
            staTableHtmlString += curr_device["station_info"][nb].station_mac;
            staTableHtmlString += '</td><td>';
            staTableHtmlString += curr_device["station_info"][nb].station_rssi;
            staTableHtmlString += '</td><td>';
            staTableHtmlString += curr_device["station_info"][nb].station_connected_band;
            staTableHtmlString += '</td><td>';
            staTableHtmlString += curr_device["station_info"][nb].station_downlink;
            staTableHtmlString += '</td><td>';
            staTableHtmlString += curr_device["station_info"][nb].station_uplink;
            staTableHtmlString += '</td></tr>';

        }
    }

    document.getElementById("nbTableContainer").innerHTML = nbTableHtmlString;
	document.getElementById("staTableContainer").innerHTML = staTableHtmlString;
}

var device_counter = 0;

function get_device_json(object_json) {
    device_counter++;
    if (device_counter == count) {
        return object_json;
    }
	// htmlString += '<li>';
	// htmlString += object_json.device_name + ' | ' + object_json.mac_address + ' | <input type="button" value="Show Details" onClick="showDetailOnClick(' + device_counter.toString() + ')">';
	if (0 != object_json["child_devices"].length) {
		for (child_device in object_json["child_devices"]) {
            var return_object = get_device_json(object_json["child_devices"][child_device]);
            if (0 == return_object) {
                continue;
            } else {
                return return_object;
            }
		}
    }

    return 0;
}

</SCRIPT>

</head>

<body onload="loadInfo();">
<div class="intro_main ">
	<p class="intro_title"><% multilang(LANG_WLAN_EASY_MESH_DEVICE_DETAILS); %></p>
	<p class="intro_content"><% multilang(LANG_WLAN_EASY_MESH_DEVICE_DETAILS_DESC); %></p>
</div>

<div class="column">
 <div class="column_title">
  <div class="column_title_left"></div>
   <p><% multilang(LANG_WLAN_EASY_MESH_NEIGHBOR_RSSI); %>:</p>
  <div class="column_title_right"></div>
 </div>

<div class="data_common">
<form action=/boafrm/formWirelessTbl method=POST name="formWirelessTbl">
<table id='nbTableContainer'>


<!-- <tr class="tbl_body"><td><font size=2>f0:25:b7:ce:57:36</td><td><font size=2>Agent1</td><td><font size=2>120</td></tr>
<tr class="tbl_body"><td><font size=2>f0:25:b7:ce:57:36</td><td><font size=2>EasyMesh_Agent</td><td><font size=2>120</td></tr> -->
<!-- <td width="60"><font size=2><b>Rx Packet</b></td>
<td width="60"><font size=2><b>Tx Rate (Mbps)</b></td>
<td width="60"><font size=2><b>Power Saving</b></td>
<td width="60"><font size=2><b>Expired Time (s)</b></td></tr> -->
</table>
</div>
</div>

<div class="column">
 <div class="column_title">
  <div class="column_title_left"></div>
   <p><% multilang(LANG_WLAN_EASY_MESH_STA_INFO); %>:</p>
  <div class="column_title_right"></div>
</div>

<div class="data_common">
<table id='staTableContainer'>
<!--
<td width="250"><font size=2><b>SSID</b></td>
-->
<!-- <td width="60"><font size=2><b>Mode</b></td> -->

<!-- <tr class="tbl_body"><td><font size=2>f0:25:b7:ce:57:36</td><td><font size=2>120</td><td><font size=2>5G</td><td><font size=2>100</td><td><font size=2>50</td></tr>
<tr class="tbl_body"><td><font size=2>f0:25:b7:ce:57:36</td><td><font size=2>120</td><td><font size=2>5G</td><td><font size=2>100</td><td><font size=2>50</td></tr>
<tr class="tbl_body"><td><font size=2>f0:25:b7:ce:57:36</td><td><font size=2>120</td><td><font size=2>5G</td><td><font size=2>100</td><td><font size=2>50</td></tr> -->
<!-- <td width="60"><font size=2><b>Expired Time (s)</b></td></tr> -->

</table>
</div>
</div>
<div class="btn_ctl">
  <input type="hidden" value="/wlstatbl.htm" name="submit-url">
  <input type="button" value="<% multilang(LANG_REFRESH); %>" class="link_bg" onclick="location.reload(true);">&nbsp;&nbsp;
  <input type="button" value="<% multilang(LANG_CLOSE); %>" class="link_bg" name="close" onClick="javascript: window.close();">
</div>
</form>
</body>

</html>
