<% SendWebHeadStr();%>
<meta HTTP-EQUIV='Pragma' CONTENT='no-cache'>
<title><% multilang(LANG_NATSESSION_CONFIGURATION); %></title>
<script type="text/javascript" src="admin/md5.js"></script>
<SCRIPT>

function saveChanges(obj)
{
/*
    <% holePunghing("check-url"); %>
    <% holePunghing("check-port"); %>
    <% holePunghing("check-interval"); %>
*/
/*
    <% natSession_set("natsessioncount"); %>
    <% natSession_set("natsessionthreshold"); %>
*/

	if (isAttackXSS(document.natsession.natsessioncount.value)) {
		alert("ERROR: Please check the value again");
		document.natsession.natsessioncount.focus();
		return false;
	}

	if (isAttackXSS(document.natsession.natsessionthreshold.value)) {
		alert("ERROR: Please check the value again");
		document.natsession.natsessionthreshold.focus();
		return false;
	}

    obj.isclick = 1;
	postTableEncrypt(document.natsession.postSecurityFlag, document.natsession);
    return true;
}

function on_submit(obj)
{
	obj.isclick = 1;
	postTableEncrypt(document.natsession.postSecurityFlag, document.natsession);
	window.location.reload();
	return true;
}

</SCRIPT>
</head>

<BODY>
<div class="intro_main ">
	<p class="intro_title"><% multilang(LANG_NATSESSION_CONFIGURATION); %></p>
	<p class="intro_content"></p>
</div>
<form action=/boaform/admin/formNatSessionSetup method=POST name="natsession">
<div class="data_common data_common_notitle">
	<table>
		<tr>
			<% ShowNatSessionCount(); %>
		</tr>
		<tr>
			<% ShowNatSessionThreshold(); %>
		</tr>
		<tr>
			<th><% multilang(LANG_NATSESSION_RESET); %>:</th>
			<td><input class="link_bg" type="submit" name="natsessionreset" value="<% multilang(LANG_RESET); %>" onClick="return on_submit(this)"></td>
		</tr>
	</table>
</div>
<div class="btn_ctl clearfix">
	<input type="hidden" value="/admin/natsession.asp" name="submit-url">
	<input class="link_bg" type="submit" value="<% multilang(LANG_APPLY_CHANGES); %>" name="apply" onClick="return saveChanges(this)">&nbsp;&nbsp;
	<input class="link_bg" type="submit" value="<% multilang(LANG_REFRESH); %>" name="nat_refresh" onClick="return on_submit(this)">
	<input type="hidden" name="postSecurityFlag" value="">
</div>
</form>
<br><br>
</body>
</html>


