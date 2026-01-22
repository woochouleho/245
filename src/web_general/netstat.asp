<% SendWebHeadStr();%>
<title>Netstat Info <% multilang(LANG_DIAGNOSTICS); %></title>

<SCRIPT>
function goClick()
{
	var submit_elm=document.getElementById("netstatSubmit");

	document.getElementById("status").style.display = "";
	if (submit_elm.value=="Search")
		document.netstat.netstatAct.value="Search";
	else
		document.netstat.netstatAct.value="Reset";

	document.getElementById("netstatSubmit").value = "Reset"
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
}

function on_init(){
	<% clearNetstatResult(); %>
}
</SCRIPT>
</head>

<body onload="on_init();">
<div class="intro_main ">
	<p class="intro_title">Netstat Infor <% multilang(LANG_DIAGNOSTICS); %></p>
</div>

<form action=/boaform/formNetstat method=POST name="netstat">
<div class="data_common data_common_notitle" id="status" style="display:none">
	<iframe src="netstat_result.asp" width="100%" height="100%"></iframe>
</div>
<div class="btn_ctl">
      <input type="hidden" name="netstatAct">
      <input id="netstatSubmit" class="link_bg" type="submit" value="Search" onClick="return goClick()">
      <input type="hidden" value="/netstat.asp" name="submit-url">
      <input type="hidden" name="postSecurityFlag" value="">
</div>
</form>
<br><br>
</body>
</html>
