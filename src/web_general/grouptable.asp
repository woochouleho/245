<% SendWebHeadStr();%>
<title>Group Table <% multilang(LANG_DIAGNOSTICS); %></title>

<SCRIPT>
var user_level = 0;
<% user_confirm(); %>

function goClick()
{
	var submit_elm=document.getElementById("grouptableSubmit");

	if (user_level != 1) {
		alert("Not support account level");
		return false;
	}

	document.getElementById("status").style.display = "";
	if (submit_elm.value=="Search")
		document.grouptable.grouptableAct.value="allSearch";
	else
		document.grouptable.grouptableAct.value="allReset";

	document.getElementById("grouptableSubmit").value = "Reset"
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
}

function on_init(){
	<% clearGroupTableResult(); %>
}
</SCRIPT>
</head>

<body onload="on_init();">
<div class="intro_main ">
	<p class="intro_title">Group Table <% multilang(LANG_DIAGNOSTICS); %></p>
</div>

<form action=/boaform/formGroupTable method=POST name="grouptable">
<div class="data_common data_common_notitle" id="status" style="display:none">
	<iframe src="grouptable_result.asp" width="100%" height="100%"></iframe>
</div>
<div class="btn_ctl">
      <input type="hidden" name="grouptableAct">
      <input id="grouptableSubmit" class="link_bg" type="submit" value="Search" onClick="return goClick()">
      <input type="hidden" value="/grouptable.asp" name="submit-url">
      <input type="hidden" name="postSecurityFlag" value="">
</div>
</form>
<br><br>
</body>
</html>
