<% SendWebHeadStr();%>
<title>Tracert6 <% multilang(LANG_DIAGNOSTICS); %></title>
<script>
function on_Apply(obj)
{
	var submit_elm=document.getElementById("tracertSubmit");

	if (isAttackXSS(document.tracert.traceAddr.value)) {
		alert("ERROR: Please check the value again");
		document.tracert.traceAddr.focus();
		return false;
	}

	if (isAttackXSSCmdInjection(document.tracert.traceAddr.value)) {
		alert("ERROR: Please check the value again");
		document.tracert.traceAddr.focus();
		return false;
	}

	if (isAttackXSS(document.tracert.trys.value)) {
		alert("ERROR: Please check the value again");
		document.tracert.trys.focus();
		return false;
	}

	if (isAttackXSS(document.tracert.timeout.value)) {
		alert("ERROR: Please check the value again");
		document.tracert.timeout.focus();
		return false;
	}

	if (isAttackXSS(document.tracert.datasize.value)) {
		alert("ERROR: Please check the value again");
		document.tracert.datasize.focus();
		return false;
	}

	if (isAttackXSS(document.tracert.maxhop.value)) {
		alert("ERROR: Please check the value again");
		document.tracert.maxhop.focus();
		return false;
	}

	if(document.getElementById("traceAddr").value == "")
	{
		alert("Should input a domain or ip address!");
		document.getElementById("traceAddr").focus();
		return false;
	}

	document.getElementById("status").style.display = "";
	if (submit_elm.value=="Start")
		document.tracert.tracertAct.value="Start";
	else
		document.tracert.tracertAct.value="Stop";
	document.getElementById("tracertSubmit").value = "Stop"

	obj.isclick = 1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
	return true;
}

function on_init(){
	<% clearTracertResult(); %>
}
</script>
</head>

<body onload="on_init();">
<div class="intro_main ">
	<p class="intro_title">Traceroute6 <% multilang(LANG_DIAGNOSTICS); %></p>
	<p class="intro_content"><% multilang(LANG_PAGE_DESC_TRACERT6_DIAGNOSTIC); %></p>
</div>
<form id="form" action=/boaform/formTracert6 method=POST name="tracert">
<div class="data_common data_common_notitle">
	<table>
		<tr>
			<th width="30%"><% multilang(LANG_HOST_ADDRESS); %>: </th>
			<td width="70%"><input type="text" id="traceAddr" name="traceAddr" size="30" maxlength="50"></td>
		</tr>
		<tr>
			<th width="30%">NumberOfTries:</th>
			<td width="70%"><input type="text" id="trys" name="trys" size="5" maxlength="5" value="3"></td>
		</tr>
		<tr>
			<th width="30%">Timeout:</th>
			<td width="70%"><input type="text" id="timeout" name="timeout" size="10" maxlength="10" value="5">s</td>
		</tr>
		<tr>
			<th width="30%">Datasize:</th>
			<td width="70%"><input type="text" id="datasize" name="datasize" size="10" maxlength="10" value="56">Bytes</td>
		</tr>
		<tr>
			<th width="30%">MaxHopCount:</th>
			<td width="70%"><input type="text" id="maxhop" name="maxhop" size="10" maxlength="10" value="30"></td>
		</tr>
		<tr>
			<th width="30%"><% multilang(LANG_WAN_INTERFACE); %>: </th>
			<td width="70%"><select name="wanif"><% if_wan_list("rtv6-any-vpn"); %></select></td>
		</tr>
	</table>
</div>
<div class="data_common data_common_notitle" id="status" style="display:none">
	<iframe src="tracert6_result.asp" width="100%" height="100%"></iframe>
</div>
<div class="btn_ctl">
	<input type="hidden" name="tracertAct">
	<input id="tracertSubmit" class="link_bg" type="submit" value="Start" name="go" onClick="return on_Apply(this)">
	<input type="hidden" value="/tracert6.asp" name="submit-url">
	<input type="hidden" name="postSecurityFlag" value="">
</div>
</form>

    <br><br>
</body>
</html>
