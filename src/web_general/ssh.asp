<% SendWebHeadStr();%>
<title>Group Table <% multilang(LANG_DIAGNOSTICS); %></title>

<SCRIPT>
function goClick()
{
	var submit_elm=document.getElementById("sshSubmit");

    if (isAttackXSS(document.ssh.sshPort.value) || 
		(!validateDecimalDigit(document.ssh.sshPort.value)) || 
		(document.ssh.sshPort.value < 1 || document.ssh.sshPort.value > 65535)) 
	{
        alert("ERROR: Please check the value again");
        document.ssh.sshPort.focus();
        return false;
    }

	document.getElementById("status").style.display = "";

	if (submit_elm.value == "Enable")
		document.ssh.sshAct.value = "Enable";
	else
		document.ssh.sshAct.value = "Disable";

	document.getElementById("sshSubmit").value = "Disable"

	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);

	return true;
}

function on_init(){
	<% clearSshResult(); %>
}
</SCRIPT>
</head>

<body onload="on_init();">
<div class="intro_main ">
	<p class="intro_title">SSH <% multilang(LANG_DIAGNOSTICS); %></p>
</div>

<form action=/boaform/formSsh method=POST name="ssh">
<div class="data_common data_common_notitle">
	<table>
		<tr>
			<% sshPortGet(); %>
		</tr>
	</table>
</div>
<div class="data_common data_common_notitle" id="status" style="display:none">
	<iframe src="ssh_result.asp" width="100%" height="100%"></iframe>
</div>
<div class="btn_ctl">
      <input type="hidden" name="sshAct">
      <input id="sshSubmit" class="link_bg" type="submit" value="Enable" onClick="return goClick()">
      <input type="hidden" value="/ssh.asp" name="submit-url">
      <input type="hidden" name="postSecurityFlag" value="">
</div>
</form>
<br><br>
</body>
</html>
