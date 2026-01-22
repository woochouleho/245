<% SendWebHeadStr();%>
<title><% multilang(LANG_FIRMWARE_UPGRADE); %></title>
<META http-equiv=cache-control content="no-cache, must-revalidate">
<script>

function $id(id) {
    return document.getElementById(id);
}

function onClick_cbx(type)
{
	var xhr2 = new XMLHttpRequest();
	var formData = new FormData(document.forms[1]);
	var actionId;
	
	if(type == '1' && document.forms[0].upgrade_cbx.checked == true)
		actionId = "formUpload1";
	else if(type == '1' && document.forms[0].upgrade_cbx.checked == false)
		actionId = "formUpload2";
	else if(type == '2' && document.forms[0].commit_cbx.checked == true)
		actionId = "formUpload3";
	else if(type == '2' && document.forms[0].commit_cbx.checked == false)
		actionId = "formUpload4";
	else if(type == '3' && document.forms[0].reboot_cbx.checked == true)
		actionId = "formUpload5";
	else if(type == '3' && document.forms[0].reboot_cbx.checked == false)
		actionId = "formUpload6";
	else
		return;

	if (xhr2.upload) {
		xhr2.open("POST", $id(actionId).action, true);
		xhr2.onload = function() {
			if (xhr2.readyState == xhr2.DONE) {
				if (xhr2.status == 200)
					;
			}
		};      
		xhr2.send(formData);
	}
}

function sendClicked()
{
	if (document.forms[0].upgrade_cbx.checked == true)
	{
		if (document.password.binary.value=="") {
			alert("<% multilang(LANG_SELECTED_FILE_CANNOT_BE_EMPTY); %>");
			document.password.binary.focus();
			return false;
		}

		if (!confirm('<% multilang(LANG_PAGE_DESC_UPGRADE_CONFIRM); %>'))
			return false;
	}
	else
	{
		document.forms[0].submit();

		return false;
	}
	return true;
}

function initBankInfo()
{
    var swVersion0Td = document.querySelector('td[name="sw_version0"]');
    var swCommit0Td = document.querySelector('td[name="sw_commit0"]');
    var swValid0Td = document.querySelector('td[name="sw_valid0"]');
    var swActive0Td = document.querySelector('td[name="sw_active0"]');

    var swVersion1Td = document.querySelector('td[name="sw_version1"]');
    var swCommit1Td = document.querySelector('td[name="sw_commit1"]');
    var swValid1Td = document.querySelector('td[name="sw_valid1"]');
    var swActive1Td = document.querySelector('td[name="sw_active1"]');
    
    swVersion0Td.innerHTML = "<% nvramGet("sw_version0"); %>";
    swCommit0Td.innerHTML = "<% nvramGet("sw_commit0"); %>";
    swValid0Td.innerHTML = "<% nvramGet("sw_valid0"); %>";
    swActive0Td.innerHTML = "<% nvramGet("sw_active0"); %>";
    swVersion1Td.innerHTML = "<% nvramGet("sw_version1"); %>";
    swCommit1Td.innerHTML = "<% nvramGet("sw_commit1"); %>";
    swValid1Td.innerHTML = "<% nvramGet("sw_valid1"); %>";
    swActive1Td.innerHTML = "<% nvramGet("sw_active1"); %>";
}

</script>

</head>
<BODY>
<div class="intro_main ">
	<p class="intro_title"><% multilang(LANG_FIRMWARE_UPGRADE); %></p>
	<p class="intro_content"> <% multilang(LANG_PAGE_DESC_UPGRADE_FIRMWARE); %></p>
</div>
<form action=/boaform/admin/formBankStatus method=POST name="formBankStatus">
<div class="column" >
	<div class="column_title">
		<div class="column_title_left"></div>
		<p><% multilang(LANG_BANK_STATUS); %></p>
		<div class="column_title_right"></div>
	</div>
    <div class="data_common data_vertical">
        <table>
            <tr>
                <th width="5%" align=center></th>
                <th width="10%" align=center>Version</th>
                <th width="5%" align=center>Commit</th>
                <th width="5%" align=center>Valid</th>
                <th width="5%" align=center>Active</th>
            </tr>
            <tr>
                <td align=center width="5%">BANK1</td>
                <td align=center width="10%"name="sw_version0"></td>
                <td align=center width="5%" name="sw_commit0"></td>
                <td align=center width="5%" name="sw_valid0"></td>
                <td align=center width="5%" name="sw_active0"></td>
            </tr>
            <tr>
                <td align=center width="5%">BANK2</td>
                <td align=center width="10%" name="sw_version1"></td>
                <td align=center width="5%" name="sw_commit1"></td>
                <td align=center width="5%" name="sw_valid1"></td>
                <td align=center width="5%" name="sw_active1"></td>
            </tr>
        </table>
    </div>
    <div class="data_common data_common_notitle" > <!--style="display:none;"-->
        <table>
            <tr>
				<th width="5%" rowspan="2"><% multilang(LANG_UPGRADE_MODE_SELECT); %></th>
				<td width="25%">
					<input type='checkbox' name='upgrade_cbx' onClick="onClick_cbx(1)" value='0'>&nbsp;&nbsp; <% multilang(LANG_UPGRADE); %> &nbsp; &nbsp;
					<input type='checkbox' name='commit_cbx' onClick="onClick_cbx(2)" value='1'>&nbsp;&nbsp; <% multilang(LANG_BANK_COMMIT); %> &nbsp; &nbsp;
					<input type='checkbox' name='reboot_cbx' onClick="onClick_cbx(3)" value='2'>&nbsp;&nbsp; <% multilang(LANG_BANK_REBOOT); %>
				</td>
            </tr>
        </table>
    </div>
    <div class="btn_ctl" style="display:none;">
        <input class="link_bg" type="submit" value="Apply Changes" name="apply" >
        <input type="hidden" value="/admin/upgrade.asp" name="submit-url">
    </div>
</div>
</form>

<form id="formUpload1" action=/boaform/admin/formUpload1 method=POST enctype="multipart/form-data"></form>
<form id="formUpload2" action=/boaform/admin/formUpload2 method=POST enctype="multipart/form-data"></form>
<form id="formUpload3" action=/boaform/admin/formUpload3 method=POST enctype="multipart/form-data"></form>
<form id="formUpload4" action=/boaform/admin/formUpload4 method=POST enctype="multipart/form-data"></form>
<form id="formUpload5" action=/boaform/admin/formUpload5 method=POST enctype="multipart/form-data"></form>
<form id="formUpload6" action=/boaform/admin/formUpload6 method=POST enctype="multipart/form-data"></form>

<form action=/boaform/admin/formUpload method=POST enctype="multipart/form-data" 
name="password">
<div class="column" >
<div class="data_common data_common_notitle">
	<table>
		<tr>
            <th width="5%" rowspan="2"><% multilang(LANG_UPGRADE_FILE_SELECT); %></th>
            <td width="25%">
			<input class="inner_btn" type="file" value="<% multilang(LANG_CHOOSE_FILE); %>" name="binary"><!-- size=20>-->
			</td>
		</tr>
	</table>
</div> &nbsp; &nbsp;
<div class="adsl clearfix">
    <input class="link_bg" type="submit" value="<% multilang(LANG_UPGRADE); %>" name="send" onclick="return sendClicked()">&nbsp;&nbsp;
	<input class="link_bg" type="reset" value="<% multilang(LANG_RESET); %>" name="reset">
	<input type="hidden" value="/admin/upgrade.asp" name="submit-url">
</div>
</div>
</form>
<script>
initBankInfo(); 
</script>
</body>
</html>
