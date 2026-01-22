<html><head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<meta HTTP-EQUIV='Pragma' CONTENT='no-cache'>
<meta HTTP-EQUIV='Content-Type' CONTENT='text/html; charset=utf-8' />
<link rel='stylesheet' href='/admin/common.css'/>
<style type="text/css">
.button {display: inline-block;zoom: 1;*display: inline;vertical-align: baseline;margin: 0 2px; outline: none;cursor: pointer;text-align: center;text-decoration: none;font: 14px/100% Arial, Helvetica, sans-serif;padding: .5em 2em .55em;text-shadow: 0 1px 1px rgba(0,0,0,.3);-webkit-border-radius: .5em;-moz-border-radius: .5em;border-radius: .5em;-webkit-box-shadow: 0 1px 2px rgba(0,0,0,.2);-moz-box-shadow: 0 1px 2px rgba(0,0,0,.2);box-shadow: 0 1px 2px rgba(0,0,0,.2);}
.orange {color: #fef4e9;border: solid 1px #da7c0c;background: #f78d1d;background: -webkit-gradient(linear, left top, left bottom, from(#ff5a20), to(#ff5a20));background: -moz-linear-gradient(top,  #ff5a20,  #ff5a20);filter:  progid:DXImageTransform.Microsoft.gradient(startColorstr='#ff5a20', endColorstr='#ff5a20');}
.orange:hover {background: #f47c20;background: -webkit-gradient(linear, left top, left bottom, from(#f88e11), to(#f06015));background: -moz-linear-gradient(top,  #f88e11,  #f06015);filter:  progid:DXImageTransform.Microsoft.gradient(startColorstr='#f88e11', endColorstr='#f06015');}
.orange:active {color: #fcd3a5;background: -webkit-gradient(linear, left top, left bottom, from(#ff5a20), to(#ff5a20));background: -moz-linear-gradient(top,  #ff5a20,  #ff5a20);filter:  progid:DXImageTransform.Microsoft.gradient(startColorstr='#ff5a20', endColorstr='#ff5a20');}
</style>
<script language='javascript'>
function aplistreload() {
	window.location.reload()
}

var brip = '<% getInfo("lan-dhcp-gateway"); %>';
var subnet = '<% getInfo("lan-dhcpSubnetMask"); %>';

function apconClickLogin(ipaddress) {
	document.getElementById('ip_address').value = ipaddress;
	document.getElementById('ap_ip_address').value = window.location.hostname;
	document.getElementById('formWanApConnection').submit();

	//window.location.reload();

}
</script>
</head>
<body>
<div class="container">
<form action=/boaform/admin/formWanApConnection method=POST name=formWanApConnection id=formWanApConnection>
	<h1 class="title_style_1"><% multilang(LANG_AP_CONNECTION_MEUN_CONNECTION); %></h1>
	<p><% multilang(LANG_AP_CONNECTION_MEUN_CONNECTION_EXPLANATION); %></p>
	<hr>
	<div id="dhcpList" name="dhcpList">
		<h2 class="title_style_2"><% multilang(LANG_AP_CONNECTION_WIFI_LIST); %></h2>
		<div class="tb_style_2 resize">
			<table id="dhcpListTable">
				<thead>
					<tr>
						<colgroup>
							<col style='width:15%;'>
							<col style='width:20%;'>
							<col style='width:15%;'>
							<col style='width:15%;'>
							<col style='width:10%;'>
							<col style='width:10%;'>
							<col style='width:15%;'>
						</colgroup>
						<th scope='col'><center><% multilang(LANG_HOSTNAME); %></center></th>
						<th scope='col'><center><% multilang(LANG_SSID); %></center></th>
						<th scope='col'><center><% multilang(LANG_IP_ADDRESS); %></center></th>
						<th scope='col'><center><% multilang(LANG_MAC_ADDRESS); %></center></th>
						<th scope='col'><center><% multilang(LANG_PORT); %></center></th>
						<th scope='col'><center><% multilang(LANG_EXPIRED_TIME_SEC); %></center></th>
						<th scope='col'><center><% multilang(LANG_CONNECT); %></center></th>
					</tr>
				</thead>
				<tbody>
                    <% CheckApConnection("load-AP-wan"); %>

				</tbody>
			</table>
			<input type="hidden" name="ip_address" id="ip_address">
			<input type="hidden" name="ap_ip_address" id="ap_ip_address">
			<input type="hidden" value="/admin/wanApConnection.asp" name="submit-url">
		</div>
	</div>
	<div class='txt_c'>
		<input type='button' onClick='aplistreload()' value='<% multilang(LANG_REFRESH); %>'  class='basic-btn01 btn-orange2-bg'>
	</div>
</form>
</div>
</body>
</html>

