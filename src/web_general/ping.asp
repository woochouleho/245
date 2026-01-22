<% SendWebHeadStr();%>
<title>Ping <% multilang(LANG_DIAGNOSTICS); %></title>

<SCRIPT>
function goClick()
{
	var submit_elm=document.getElementById("pingSubmit");

	if (document.ping.pingAddr.value=="") {
		alert("Enter host address !");
		document.ping.pingAddr.focus();
		return false;
	}

	if (isValidIpAddressValue(document.ping.pingAddr.value)) {
		alert("ERROR: Please check the value again");
		document.ping.pingAddr.focus();
		return false;
	}

	if (isAttackXSS(document.ping.pingAddr.value)) {
		alert("ERROR: Please check the value again");
		document.ping.pingAddr.focus();
		return false;
	}

	if (isAttackXSSCmdInjection(document.ping.pingAddr.value)) {
		alert("ERROR: Please check the value again");
		document.ping.pingAddr.focus();
		return false;
	}

	document.getElementById("status").style.display = "";
	if (submit_elm.value=="Start")
		document.ping.pingAct.value="Start";
	else
		document.ping.pingAct.value="Stop";
	document.getElementById("pingSubmit").value = "Stop"
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
}

function netstat_display()
{
	const ipaddr = "<% getInfo("lan-ip"); %>";
	let li = parent.document.getElementById("netstat.asp").parentNode;
	if(li.style.display === "none")
	{
		if (window.location.hostname !== ipaddr)
			li.style.display = "";
	}
}

function on_init(){
	<% clearPingResult(); %>
	netstat_display();
}
</SCRIPT>
</head>

<body onload="on_init();">
<div class="intro_main ">
	<p class="intro_title">Ping <% multilang(LANG_DIAGNOSTICS); %></p>
	<p class="intro_content">  <% multilang(LANG_PAGE_DESC_ICMP_DIAGNOSTIC); %></p>
</div>

<form action=/boaform/formPing method=POST name="ping">
<div class="data_common data_common_notitle">
	<table>
	  <tr>
	      <th width="30%"><% multilang(LANG_HOST_ADDRESS); %>: </th>
	      <td width="70%"><input type="text" name="pingAddr" size="30" maxlength="30"></td>
	  </tr>
	  <tr>
	  	  <th width="30%"><% multilang(LANG_WAN_INTERFACE); %>: </th>
	  	  <td width="70%"><select name="wanif"><% if_wan_list("rt-any-vpn"); %></select></td>
	  </tr>
	</table>
</div>
<div class="data_common data_common_notitle" id="status" style="display:none">
	<iframe src="ping_result.asp" width="100%" height="100%"></iframe>
</div>
<div class="btn_ctl">
      <input type="hidden" name="pingAct">
      <input id="pingSubmit" class="link_bg" type="submit" value="Start" onClick="return goClick()">
      <input type="hidden" value="/ping.asp" name="submit-url">
      <input type="hidden" name="postSecurityFlag" value="">
</div>
</form>
<br><br>
</body>
</html>
