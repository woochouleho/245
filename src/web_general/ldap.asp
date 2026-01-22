<%SendWebHeadStr(); %>
<title><% multilang(LANG_LDAP); %></title>

<script language="javascript">

var ldap_enable = '<% getInfo("ldap_enable"); %>';
var ldap_url = '<% getInfo("ldap_url"); %>';
var ldap_file = '<% getInfo("ldap_file"); %>';
var ldap_relativepath = '<% getInfo("ldap_relativepath"); %>';

function clearldapgroupValue()
{
    document.formLdap.ldap_url.value = '';
    document.formLdap.ldap_file.value = '';
    document.formLdap.ldap_relativepath.value = '';
}

function setldapgroupValue()
{
    document.formLdap.ldap_url.value = ldap_url;
    document.formLdap.ldap_file.value = ldap_file;
    document.formLdap.ldap_relativepath.value = ldap_relativepath;
}

function changeldapstatus()
{
    if (document.formLdap.ldapcap[0].checked == true)
    {
        //document.getElementById('ldapgroup').style.display = 'none';
        changeBlockState('ldapgroup', true);
        clearldapgroupValue();
    }
    else if (document.formLdap.ldapcap[1].checked == true)
    {
        //document.getElementById('ldapgroup').style.display = '';
        changeBlockState('ldapgroup', false);
        setldapgroupValue();
    }
}

function initldap()
{
    if (ldap_enable == 1)
    {
        document.formLdap.ldapcap[1].checked = true;
        //document.getElementById('ldapgroup').style.display = '';
        changeBlockState('ldapgroup', false);
        setldapgroupValue();
    }
    else if (ldap_enable == 0)
    {
        document.formLdap.ldapcap[0].checked = true;
        //document.getElementById('ldapgroup').style.display = 'none';
        changeBlockState('ldapgroup', true);

        clearldapgroupValue();
    }
}


function saveClick(obj)
{
    if(isURLWithPortNum(document.formLdap.ldap_url) == false)
        return false;

    if(isInvalidPath(document.formLdap.ldap_file) == false)
        return false;

    if(isInvalidPath(document.formLdap.ldap_relativepath) == false)
        return false;

	if (isAttackXSS(document.formLdap.ldap_url.value)) 
	{
		alert("ERROR: Please check the value again");
		return false;
	}
	if (isAttackXSS(document.formLdap.ldap_file.value)) 
	{
		alert("ERROR: Please check the value again");
		return false;
	}
	if (isAttackXSS(document.formLdap.ldap_relativepath.value)) 
	{
		alert("ERROR: Please check the value again");
		return false;
	}

	obj.isclick = 1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
	return true;
}

</script>
</head>

<body>
<div class="intro_main ">
	<p class="intro_title"><% multilang(LANG_LDAP); %></p>
</div>

<form action=/boaform/admin/formLdap method=POST name=formLdap>
<div class="data_common data_common_notitle">
	<table>
		<tr>
			<th width=30%><% multilang(LANG_LDAP); %>:&nbsp;</th>
			<td>
				<input type="radio" value="0" name="ldapcap" onClick='changeldapstatus()' ><% multilang(LANG_DISABLE); %>&nbsp;&nbsp;
				<input type="radio" value="1" name="ldapcap" onClick='changeldapstatus()' ><% multilang(LANG_ENABLE); %>
			</td>
		</tr>    
		<TBODY id='ldapgroup'>
            <tr>
                <th><% multilang(LANG_LDAP_URL); %>:&nbsp;</th>
                <td><input type='text' name='ldap_url' maxlength="50" size="35"></td>
            </tr>
            <tr>
                <th><% multilang(LANG_LDAP_FILE); %>:&nbsp;</th>
                <td><input type='text' name='ldap_file' maxlength="50" size="35"></td>
            </tr>
            <tr>
                <th><% multilang(LANG_LDAP_UPG_RELATIVEPATH); %>:&nbsp;</th>
                <td><input type='text' name='ldap_relativepath' maxlength="50" size="35"></td>
            </tr>

		</TBODY>
	</table>
</div>

<div class="btn_ctl">
	<input class="link_bg" type="submit" value="<% multilang(LANG_APPLY_CHANGES); %>" name="apply" onClick="return saveClick(this)">
	<input type="hidden" value="/admin/ldap.asp" name="submit-url">
	<input type="hidden" name="postSecurityFlag" value="">
</div>

</form>
<script>
initldap();
</script>
</blockquote>
</body>
</html>


