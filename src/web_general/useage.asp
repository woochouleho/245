<% SendWebHeadStr();%>
<meta HTTP-EQUIV='Pragma' CONTENT='no-cache'>
<title><% multilang(LANG_USEAGE_CONFIGURATION); %></title>
<script type="text/javascript" src="admin/md5.js"></script>
<SCRIPT>

function saveChanges(obj)
{
	if (checkDigitRange(document.useage.cpuusagethreshold.value, 1, 0, 100) == 0) {
		alert("<% multilang(LANG_INVALID_USEAGE_START_RANGE_IT_SHOULD_BE_0_100); %>");
		document.useage.cpuusagethreshold.focus();
		return false;
	}

	if (checkDigitRange(document.useage.memusagethreshold.value, 1, 0, 100) == 0) {
		alert("<% multilang(LANG_INVALID_USEAGE_START_RANGE_IT_SHOULD_BE_0_100); %>");
		document.useage.cpuusagethreshold.focus();
		return false;
	}

	if (checkDigitRange(document.useage.flashusagethreshold.value, 1, 0, 100) == 0) {
		alert("<% multilang(LANG_INVALID_USEAGE_START_RANGE_IT_SHOULD_BE_0_100); %>");
		document.useage.flashusagethreshold.focus();
		return false;
	}


	if (checkDigit(document.useage.cpuusagethreshold.value) == 0) {
		alert("<% multilang(LANG_IT_SHOULD_BE_IN_NUMBER_0_9); %>");
		document.useage.cpuusagethreshold.focus();
		return false;
	}

	if (checkDigit(document.useage.memusagethreshold.value) == 0) {
		alert("<% multilang(LANG_IT_SHOULD_BE_IN_NUMBER_0_9); %>");
		document.useage.memusagethreshold.focus();
		return false;
	}

	if (checkDigit(document.useage.flashusagethreshold.value) == 0) {
		alert("<% multilang(LANG_IT_SHOULD_BE_IN_NUMBER_0_9); %>");
		document.useage.flashusagethreshold.focus();
		return false;
	}

	if (isAttackXSS(document.useage.cpuusagethreshold.value)) {
		alert("ERROR: Please check the value again");
		document.useage.cpuusagethreshold.focus();
		return false;
	}

	if (isAttackXSS(document.useage.memusagethreshold.value)) {
		alert("ERROR: Please check the value again");
		document.useage.memusagethreshold.focus();
		return false;
	}

	if (isAttackXSS(document.useage.flashusagethreshold.value)) {
		alert("ERROR: Please check the value again");
		document.useage.memusagethreshold.focus();
		return false;
	}

	obj.isclick = 1;
	postTableEncrypt(document.useage.postSecurityFlag, document.useage);
	return true;
}

</SCRIPT>
</head>

<BODY>
<div class="intro_main ">
	<p class="intro_title"><% multilang(LANG_USEAGE_CONFIGURATION); %></p>
	<p class="intro_content"></p>
</div>
<form action=/boaform/admin/formUseageSetup method=POST name="useage">
<div class="data_common data_common_notitle">
	<table>
		<tr>
			<% ShowUseageThreshold(); %>
		</tr>
	</table>
</div>   
<div class="btn_ctl clearfix">
	<input type="hidden" value="/admin/useage.asp" name="submit-url">
	<input class="link_bg" type="submit" value="<% multilang(LANG_APPLY_CHANGES); %>" name="apply" onClick="return saveChanges(this)">
	<input type="hidden" name="postSecurityFlag" value="">
</div>
</form>
<br><br>
</body>
</html>
