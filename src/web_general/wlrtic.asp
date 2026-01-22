<% SendWebHeadStr();%>
<title><% multilang(LANG_WLAN_RTIC); %></title>
<SCRIPT>
var AjaxReq = null;
var mode = <% checkWrite("Change_RTIC_Mode"); %>;

function ZWDFSModeChange(){
	document.Rtic.rticmode[0].checked = true;
	document.Rtic.rticmode.value=0;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
	document.forms[0].submit();
}

function DCSModeChange(){
	document.Rtic.rticmode[1].checked = true;
	document.Rtic.rticmode.value=1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
	document.forms[0].submit();
}

function update_page()
{
	if (AjaxReq != null && AjaxReq.readyState==4 && AjaxReq.status==200)
	{
		var i;
		var loading, channel, avail;
		var xmlDoc = AjaxReq.responseXML;
		var x = xmlDoc.getElementsByTagName("entry");
		var wlan_index = <% checkWrite("wlan_idx"); %>;
		var chan = <% checkWrite("RTIC_channel_number"); %>;
		var FLAG_NON_CAC = 0;
		var table_string = "";
		var cac_string = "";
		
		cac_string += "<tr><th><font color=\"red\">" + "<% multilang(LANG_CAC_IS_GOING); %>" + "</font></th></tr>";
		table_string += "<tr><th>channel</th><th>loading</th><th>clarity</th></tr>";
		for(i = 0; i < x.length; i++){
			
			channel = x[i].getElementsByTagName("channel")[0].childNodes[0].nodeValue;
			loading = x[i].getElementsByTagName("loading")[0].childNodes[0].nodeValue;

			if(mode == 0 && wlan_index == 0){
				avail = x[i].getElementsByTagName("avail")[0].childNodes[0].nodeValue;
				if(avail == 1){
					FLAG_NON_CAC = 1;
					if(channel == chan)
						table_string += "<tr><td style=\"background-color:lightblue\">" + x[i].getElementsByTagName("channel")[0].childNodes[0].nodeValue + "(current)</td><td style=\"background-color:lightblue\">" + x[i].getElementsByTagName("loading")[0].childNodes[0].nodeValue + "</td><td style=\"background-color:lightblue\">";
					else{
						if(channel >= 52 && channel <=144)
							table_string += "<tr><td>" + x[i].getElementsByTagName("channel")[0].childNodes[0].nodeValue + "(DFS)</td><td>" + x[i].getElementsByTagName("loading")[0].childNodes[0].nodeValue + "</td><td>";
						else
							table_string += "<tr><td>" + x[i].getElementsByTagName("channel")[0].childNodes[0].nodeValue + "</td><td>" + x[i].getElementsByTagName("loading")[0].childNodes[0].nodeValue + "</td><td>";
					}	
				}
				else
					continue;
			}
			else{
				if(channel == chan)
					table_string += "<tr><td style=\"background-color:lightblue\">" + x[i].getElementsByTagName("channel")[0].childNodes[0].nodeValue + "(current)</td><td style=\"background-color:lightblue\">" + x[i].getElementsByTagName("loading")[0].childNodes[0].nodeValue + "</td><td style=\"background-color:lightblue\">";
				else
					table_string += "<tr><td>" + x[i].getElementsByTagName("channel")[0].childNodes[0].nodeValue + "</td><td>" + x[i].getElementsByTagName("loading")[0].childNodes[0].nodeValue + "</td><td>";
			}
			
			if(loading <= 5)
				table_string += "<img src=\"graphics/nice.png\" width=\"20px\" height=\"20px\" /><img src=\"graphics/nice.png\" width=\"20px\" height=\"20px\" /></td></tr>";
			if(loading >= 6 && loading <= 12)
				table_string += "<img src=\"graphics/nice.png\" width=\"20px\" height=\"20px\" /></td></tr>";
			if(loading >= 13)
				table_string += "<img src=\"graphics/bad.png\" width=\"20px\" height=\"20px\" /></td></tr>";
		}
		
		if(mode == 0 && wlan_index == 0){
			if(FLAG_NON_CAC == 1)
				document.getElementById("rtic_form").innerHTML = table_string;
			else
				document.getElementById("cac_is_going_form").innerHTML = cac_string;
		}
		else if(mode == 0 && wlan_index == 1)
			document.getElementById("RTIC_INFO").style.display="none";
		else
			document.getElementById("rtic_form").innerHTML = table_string;	

	}
}

function createRequest()
{
	var request = null;
	try { request = new XMLHttpRequest(); }
	catch (trymicrosoft)
	{
		try { request = new ActiveXObject("Msxml2.XMLHTTP"); }
		catch (othermicrosoft)
		{
			try { request = new ActiveXObject("Microsoft.XMLHTTP"); }
			catch (failed)
			{
				request = null;
			}
		}
	}
	if (request == null) alert("Error creating request object !");
	return request;
}

function send_request(url)
{
	if (AjaxReq == null) AjaxReq = createRequest();
	AjaxReq.open("GET", url, true);
	AjaxReq.onreadystatechange = update_page;
	AjaxReq.send(null);
}

function generate_random_str()
{
	var d = new Date();
	var str=d.getFullYear()+"."+(d.getMonth()+1)+"."+d.getDate()+"."+d.getHours()+"."+d.getMinutes()+"."+d.getSeconds();
	return str;
}

function update_state()
{
	send_request("wlrtic_url.asp?t="+generate_random_str());
}

function refresh_rtic_page()
{
        update_state();
        setInterval(update_state, 30000);
}
function on_init()
{
	var wlan_index = <% checkWrite("wlan_idx"); %>;
	if(mode == 0){
		if(wlan_index == 1)
			document.getElementById("refresh").style.display="none";
		document.Rtic.rticmode[0].checked = true;
		document.Rtic.rticmode[1].checked = false;
	}
	if(mode == 1){
		document.Rtic.rticmode[0].checked = false;
		document.Rtic.rticmode[1].checked = true;
	}
	refresh_rtic_page();
}


</SCRIPT>
</head>

<body onload="on_init();">
<div class="intro_main ">
        <p class="intro_title"><% multilang(LANG_WLAN_RTIC); %></p>
        <p class="intro_content"><% multilang(LANG_WLAN_RTIC_INFO); %></p>
</div>
<form action=/boaform/admin/formWlRtic method=POST name="Rtic">
<div id="mode_div" style="display:">
<div class="column">
	<div class="data_common data_common_notitle">
	<% checkWrite("RTIC_Mode_Select"); %>
	</div>
</div>
</div>

<div class="column" id="RTIC_INFO" style="display:">
	<div class="column_title">
		<div class="column_title_left"></div>
			<% checkWrite("RTIC_Mode_Title"); %>
		<div class="column_title_right"></div>
	</div>
	<div class="data_common">
	<table id="cac_is_going_form"></table>
	<table id="rtic_form"></table>
	</div>
</div>

<div id="click_div" style="display:">
<div class="btn_ctl">
        <input type="button" value="<% multilang(LANG_REFRESH); %>" class="link_bg" id="refresh" onclick="location.reload(true);">
        <input type="hidden" value="/wlrtic.asp" name="submit-url">
	  	<input type="hidden" name="postSecurityFlag" value="">
</div>
</div>
 </form>
</body>

</html>

