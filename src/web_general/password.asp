<% SendWebHeadStr();%>
<meta HTTP-EQUIV='Pragma' CONTENT='no-cache'>
<title><% multilang(LANG_PASSWORD_CONFIGURATION); %></title>
<script type="text/javascript" src="admin/md5.js"></script>
<SCRIPT>

var password_encrypt_flag= <% checkWrite("password_encrypt_flag"); %>;
var realm = <% checkWrite("realm"); %>;
var username = <% checkWrite("username"); %>;
var susername = <% checkWrite("susername"); %>;
var username_cal = "";

function saveChanges(obj)
{
	var decimal = /^(?=.*\d)(?=.*[^a-zA-Z0-9])(?!.*\s).{8,15}$/;
	var lower = /^(?=.*[a-z])/;
	var upper = /^(?=.*[A-Z])/;
	var lo = 0;
	var up = 0;

	if (isAttackXSS(document.password.oldpass.value)) {
		alert("ERROR: Please check the value again");
		document.password.oldpass.focus();
		return false;
	}
	if (isAttackXSS(document.password.newpass.value)) {
		alert("ERROR: Please check the value again");
		document.password.newpass.focus();
		return false;
	}
	if (isAttackXSS(document.password.confpass.value)) {
		alert("ERROR: Please check the value again");
		document.password.confpass.focus();
		return false;
	}

	if (document.password.newpass.value != document.password.confpass.value) {	
		alert('<% multilang(LANG_PASSWORD_IS_NOT_MATCHED_PLEASE_TYPE_THE_SAME_PASSWORD_BETWEEN_NEW_AND_CONFIRMED_BOX); %>');
		document.password.newpass.focus();
		return false;
	}

	if (document.password.newpass.value.match(decimal)) {
		/* success case */
	} else {
		alert('<% multilang(LANG_PASSWORD_LENGTH_UPPER_LOWER_NUMERIC_SPRICAL_CHARCTER_RULE); %>');
		return false;
	}

	if (document.password.newpass.value.match(lower)) {
		/* success case */
	} else {
		lo = 1;
	}

	if (document.password.newpass.value.match(upper)) {
		/* success case */
	} else {
		up = 1;
	}

	if (lo == 1 && up == 1) {
		alert('<% multilang(LANG_PASSWORD_LENGTH_UPPER_LOWER_NUMERIC_SPRICAL_CHARCTER_RULE); %>');
		return false;
	}

	if (includeSpace(document.password.newpass.value)) {
		alert("<% multilang(LANG_CANNOT_ACCEPT_SPACE_CHARACTER_IN_PASSWORD_PLEASE_TRY_IT_AGAIN); %>");
		document.password.newpass.focus();
		return false;
	}

	if (password_encrypt_flag) {
		if(document.password.userMode.value == "1")
			username_cal = username;
		else
			username_cal = susername;

		document.password.oldpass.value = hex_md5(username_cal + ":"  + realm + ":" + document.password.oldpass.value);
		document.password.newpass.value = hex_md5(username_cal + ":"  + realm + ":" + document.password.newpass.value);
		document.password.confpass.value = hex_md5(username_cal + ":"  + realm + ":" + document.password.confpass.value);
	}

	obj.isclick = 1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);

	return true;
}

</SCRIPT>
</head>

<BODY>
<div class="intro_main ">
	<p class="intro_title"><% multilang(LANG_PASSWORD_CONFIGURATION); %></p>
	<p class="intro_content">  <% multilang(LANG_PAGE_DESC_SET_ACCOUNT_PASSWORD); %></p>
</div>
<form action=/boaform/formPasswordSetup method=POST name="password">
<div class="data_common data_common_notitle">
	<table>
		<tr>
			<th width="40%"><% multilang(LANG_USER); %><% multilang(LANG_NAME); %>:</th>
			<td><select size="1" name="userMode">
				<option selected value="1">admin</option>
				</select>
			</td>
		</tr>
		<tr>
			<th><% multilang(LANG_OLD_PASSWORD); %>:</th>
			<td><input type="password" name="oldpass" size="20" maxlength="30"></td>
		</tr>
		<tr>
			<th><% multilang(LANG_NEW_PASSWORD); %>:</th>
			<td><input type="password" name="newpass" size="20" maxlength="30"></td>
		</tr>
		<tr>
			<th><% multilang(LANG_CONFIRMED_PASSWORD); %>:</th>
			<td><input type="password" name="confpass" size="20" maxlength="30"></td>
		</tr>
	</table>
</div>
<div class="btn_ctl clearfix">
	<input type="hidden" value="/password.asp" name="submit-url">
	<input class="link_bg" type="submit" value="<% multilang(LANG_APPLY_CHANGES); %>" name="save" onClick="return saveChanges(this)">&nbsp;&nbsp;
	<input class="link_bg" type="reset" value="  <% multilang(LANG_RESET); %>  " name="reset">
	<input type="hidden" name="postSecurityFlag" value="">
</div>
</form>
<br><br>
</body>
</html>


