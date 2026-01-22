<% SendWebHeadStr();%>
<title><% multilang(LANG_WLAN_EASY_MESH_TOPOLOGY); %></title>
<style>
.on {display:on}
.off {display:none}
</style>
<style type="text/css">
ul.square {
	list-style-type:square;
	padding: 0 20px;
}
</style>
<SCRIPT>
//var wlan_num =<% getIndex("show_wlan_num"); %> ;

//var wlan1Disabled = 0, wlan2Disabled = 0;
var role=<% getInfo("multi_ap_controller"); %>;

function openWindow(url, windowName, wide, high) {
	if (document.all)
		var xMax = screen.width, yMax = screen.height;
	else if (document.layers)
		var xMax = window.outerWidth, yMax = window.outerHeight;
	else
		var xMax = 640, yMax=500;
	var xOffset = (xMax - wide)/2;
	var yOffset = (yMax - high)/3;

	var settings = 'width='+wide+',height='+high+',screenX='+xOffset+',screenY='+yOffset+',top='+yOffset+',left='+xOffset+', resizable=yes, toolbar=no,location=no,directories=no,status=no,menubar=no,scrollbars=yes';
	window.open(url, windowName, settings);
}

function showDetailOnClick(count) {
	openWindow('/multi_ap_popup_device_details.asp?count='+count, 'showDeviceDetail', 700, 500);
}

var htmlString = "";
var device_counter = 0;

function print_device_json(object_json) {
	device_counter++;
	htmlString += '<li>';
	htmlString += object_json.device_name + ' | ' + object_json.mac_address + ' | ' + object_json.ip_addr + ' | <input type="button" value="Show Details" class="link_bg" onClick="showDetailOnClick(' + device_counter.toString() + ')">';
	if (0 != object_json["child_devices"].length) {
		htmlString += '<ul class="square">';
		for (child_device in object_json["child_devices"]) {
			print_device_json(object_json["child_devices"][child_device]);
		}
		htmlString += '</ul>';
	}
	htmlString += '</li>';
}

function loadInfo()
{
	if(role == 1){
		var string_json = '<% checkWrite("topology_json_string"); %>';
		var object_json = JSON.parse(string_json);
		print_device_json(object_json);
		document.getElementById("topology_insertion").innerHTML = htmlString;
		document.getElementById("controller_div").style.display = "";
	}
	else{
		document.getElementById("other_div").style.display = "";
	}
    setInterval(function(){ location.reload(true); }, 10000);
}


function resetClick()
{
	location.reload(true);
}



function saveChanges()
{
	// var dot11kvDisabled = <% getIndex("is_dot11kv_disabled"); %>;
	// if (dot11kvDisabled && (!document.getElementById("role_disabled").checked)) {
	// 	if(!confirm("11k or/and 11v are not enabled in some interface(s), click OK to enable and continue")){
	// 		return false;
	// 	}
	// 	document.getElementById("needEnable11kv").value = "1";
	// }

	// var securitySettingWrong = <% getIndex("is_security_setting_wrong"); %>;
	// if (securitySettingWrong && (role == 1)) {
	// 	alert("Something wrong with your security settings.\nPlease be aware that EasyMesh only supports WPA2 with PSK.\nPlease ensure security type is WPA2 and password is not null.");
	// 	return false;
	// }

	// if (<% getIndex("needPopupBackhaul"); %> && (role == 1)) {
	// 	if(!confirm("Alert: VAP1 for Wlan1 and/or Wlan2 will be auto-managed. \nClick OK to continue.")){
	// 		return false;
	// 	}
	// }

    return true;
}


</SCRIPT>
</head>

<body onload="loadInfo();">
<div class="intro_main ">
	<p class="intro_title"><% multilang(LANG_WLAN_EASY_MESH_TOPOLOGY); %></p>
	<p class="intro_content"><% multilang(LANG_WLAN_EASY_MESH_TOPOLOGY_DESC); %></p>
</div>

<form action=/boaform/formMultiAP method=POST name="MultiAP">
<div id="other_div" style="display:none">
<div class="data_common data_common_notitle">
<table><tr><td>
<% multilang(LANG_WLAN_EASY_MESH_ROLE_IS_NOT_CONTROLLER); %>
</td></tr></table>
</div>
</div>
<div id="controller_div" style="display:none">
<div class="data_common data_common_notitle">
<table>

  <!-- <tr>
	<td width="100%" colspan=2><font size=2><b>
	 <input type="checkbox" name="" value="ON" ONCLICK="">&nbsp;&nbsp;Disable EasyMesh</b>
	</td>
	</tr> -->

  	<tr>
		<th><% multilang(LANG_NETWORK_TOPOLOGY); %>:</th>
	</tr>
	<tr>
	<td>
	<ul id="topology_insertion" class="square">

	</ul>
	</td>
	</tr>
</table>
</div>
<div class="btn_ctl">
	<input type="button" value="<% multilang(LANG_REFRESH); %>" class="link_bg" onclick="location.reload(true);">
</div>
</div>
 </form>
</body>

</html>
