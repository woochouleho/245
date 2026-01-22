<% SendWebHeadStr();%>
<title>Ping6 <% multilang(LANG_DIAGNOSTICS); %></title>
<SCRIPT>
function goClick(obj)
{
	var submit_elm=document.getElementById("pingSubmit");

	/*if (!checkHostIP(document.ping.pingAddr, 1))
		return false;
	*/
	if (document.ping.pingAddr.value=="") {
		alert("Enter host address !");
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

	obj.isclick = 1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
	return true;
}

function on_init(){
	<% clearPingResult(); %>
}
</SCRIPT>
</head>

<body onload="on_init();">
<div class="intro_main ">
	<p class="intro_title">Ping6 <% multilang(LANG_DIAGNOSTICS); %></p>
	<p class="intro_content">   <% multilang(LANG_PAGE_DESC_ICMPV6_DIAGNOSTIC); %></p>
</div>

<form action=/boaform/formPing6 method=POST name="ping">

<div class="data_common data_common_notitle">
	<table>
		<tr>
			<th width="30%"><% multilang(LANG_HOST_ADDRESS); %>: </th>
			<td width="70%"><input type="text" name="pingAddr" size="30" maxlength="50"></td>
		</tr>
		<tr>
	  	  <th width="30%"><% multilang(LANG_WAN_INTERFACE); %>: </th>
	  	  <td width="70%"><select name="wanif"><% if_wan_list("rtv6-any-vpn"); %></select></td>
	  </tr>
	</table>
</div>
<div class="data_common data_common_notitle" id="status" style="display:none">
	<iframe src="ping6_result.asp" width="100%" height="100%"></iframe>
</div>
<div class="btn_ctl">
    <input type="hidden" name="pingAct">
	<input id="pingSubmit" class="link_bg" type="submit" value="Start" name="go" onClick="return goClick(this)">
	<input type="hidden" value="/ping6.asp" name="submit-url">
	<input type="hidden" name="postSecurityFlag" value="">
</div>
 </form>

</body>

</html>

