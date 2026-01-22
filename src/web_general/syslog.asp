<%SendWebHeadStr(); %>
<title><% multilang(LANG_SYSTEM_LOG); %></title>

<script language="javascript">
var addr1 = '<% getInfo("syslog-server-ip1"); %>';
var addr2 = '<% getInfo("syslog-server-ip2"); %>';
var addr3 = '<% getInfo("syslog-server-ip3"); %>';
var addr4 = '<% getInfo("syslog-server-ip4"); %>';
var addr5 = '<% getInfo("syslog-server-ip5"); %>';
var url   = '<% getInfo("syslog-server-url"); %>';
var port1 = '<% getInfo("syslog-server-port1"); %>';
var port2 = '<% getInfo("syslog-server-port2"); %>';
var port3 = '<% getInfo("syslog-server-port3"); %>';
var port4 = '<% getInfo("syslog-server-port4"); %>';
var port5 = '<% getInfo("syslog-server-port5"); %>';
var urlPort = '<% getInfo("syslog-server-url-port"); %>';
var logline = '<% getInfo("log-line"); %>';
function getLogline() {
	var line;
		
	line = parseInt(logline);

	if (isNaN(line) || line == 0)
		line = 6000; // default system log line is 6000

	return line;
}
function getLogPort(idx) {
	var portNum;
		
	if (idx == 1) portNum = parseInt(port1);
	else if (idx == 2) portNum = parseInt(port2);
	else if (idx == 3) portNum = parseInt(port3);
	else if (idx == 4) portNum = parseInt(port4);
	else if (idx == 5) portNum = parseInt(port5);
	else		  portNum = parseInt(urlPort);

	if (isNaN(portNum) || portNum == 0)
		portNum = 514; // default system log server port is 514

	return portNum;
}

function hideInfo(hide) {
	var status = 'visible';

	if (hide == 1) {
		status = 'hidden';
		document.forms[0].logAddr1.value = '';
		document.forms[0].logPort1.value = '';

		document.forms[0].logAddr2.value = '';
		document.forms[0].logPort2.value = '';

		document.forms[0].logAddr3.value = '';
		document.forms[0].logPort3.value = '';

		document.forms[0].logAddr4.value = '';
		document.forms[0].logPort4.value = '';

		document.forms[0].logAddr5.value = '';
		document.forms[0].logPort5.value = '';

		document.forms[0].logUrl.value = '';
		document.forms[0].logUrlPort.value = '';
		changeBlockState('srvInfo', true);
		//document.forms[0].logline.value = ''; 
	} else {
		changeBlockState('srvInfo', false);
		document.forms[0].logAddr1.value = addr1;
		document.forms[0].logPort1.value = getLogPort(1);

		document.forms[0].logAddr2.value = addr2;
		document.forms[0].logPort2.value = getLogPort(2);

		document.forms[0].logAddr3.value = addr3;
		document.forms[0].logPort3.value = getLogPort(3);

		document.forms[0].logAddr4.value = addr4;
		document.forms[0].logPort4.value = getLogPort(4);

		document.forms[0].logAddr5.value = addr5;
		document.forms[0].logPort5.value = getLogPort(5);

		document.forms[0].logUrl.value	 = url;
		document.forms[0].logUrlPort.value = getLogPort(6);
		//document.forms[0].logline.value = getLogline();

	}
}

function hidesysInfo(hide) {
	var status = false;

	if (hide == 1) {
		status = true;
	}
	changeBlockState('sysgroup', status);
}

function changelogstatus() {
	with (document.forms[0]) {
		if (logcap[1].checked) {
			hidesysInfo(0);
			if (logMode.selectedIndex == 0) {
				hideInfo(1);
			} else {
				hideInfo(0);
			}
		} else {
			hidesysInfo(1);
			hideInfo(1);
		}
	}
}

function cbClick(obj) {
	var idx = obj.selectedIndex;
	var val = obj.options[idx].value;
	
	/* 1: Local, 2: Remote, 3: Both */
	if (val == 1)
		hideInfo(1);
	else
		hideInfo(0);
}

function check_enable()
{
	if (document.formSysLog.logcap[0].checked) {
		//disableTextField(document.formSysLog.msg);
		disableButton(document.formSysLog.refresh);		
	}
	else {
		//enableTextField(document.formSysLog.msg);
		enableButton(document.formSysLog.refresh);
	}
}               

/*function scrollElementToEnd (element) {
   if (typeof element.scrollTop != 'undefined' &&
       typeof element.scrollHeight != 'undefined') {
     element.scrollTop = element.scrollHeight;
   }
}*/

function saveClick(obj)
{

	if (isAttackXSS(document.forms[0].logline.value)) {
		alert("ERROR: Please check the value again");
		document.forms[0].logline.focus();
		return false;
	}

	if (isAttackXSS(document.forms[0].logAddr1.value)) {
		alert("ERROR: Please check the value again");
		document.forms[0].logAddr1.focus();
		return false;
	}

	if (isAttackXSS(document.forms[0].logPort1.value)) {
		alert("ERROR: Please check the value again");
		document.forms[0].logPort1.focus();
		return false;
	}

	if (isAttackXSS(document.forms[0].logAddr2.value)) {
		alert("ERROR: Please check the value again");
		document.forms[0].logAddr2.focus();
		return false;
	}

	if (isAttackXSS(document.forms[0].logPort2.value)) {
		alert("ERROR: Please check the value again");
		document.forms[0].logPort2.focus();
		return false;
	}

	if (isAttackXSS(document.forms[0].logAddr3.value)) {
		alert("ERROR: Please check the value again");
		document.forms[0].logAddr3.focus();
		return false;
	}

	if (isAttackXSS(document.forms[0].logPort3.value)) {
		alert("ERROR: Please check the value again");
		document.forms[0].logPort3.focus();
		return false;
	}


	if (isAttackXSS(document.forms[0].logAddr4.value)) {
		alert("ERROR: Please check the value again");
		document.forms[0].logAddr4.focus();
		return false;
	}

	if (isAttackXSS(document.forms[0].logPort4.value)) {
		alert("ERROR: Please check the value again");
		document.forms[0].logPort4.focus();
		return false;
	}

	if (isAttackXSS(document.forms[0].logAddr5.value)) {
		alert("ERROR: Please check the value again");
		document.forms[0].logAddr5.focus();
		return false;
	}

	if (isAttackXSS(document.forms[0].logPort5.value)) {
		alert("ERROR: Please check the value again");
		document.forms[0].logPort5.focus();
		return false;
	}

	if (isAttackXSS(document.forms[0].logUrl.value)) {
		alert("ERROR: Please check the value again");
		document.forms[0].logUrl.focus();
		return false;
	}

	if (isAttackXSS(document.forms[0].logUrlPort.value)) {
		alert("ERROR: Please check the value again");
		document.forms[0].logUrlPort.focus();
		return false;
	}

	<% RemoteSyslog("check-ip"); %>
	<% RemoteSyslog("check-port"); %>
	<% RemoteSyslog("check-url"); %>
	<% RemoteSyslog("check-logline"); %>
//	if (document.forms[0].logAddr.disabled == false && !checkIP(document.formSysLog.logAddr))
//		return false;
//	alert("Please commit and reboot this system for take effect the System log!");
	obj.isclick = 1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
	return true;
}

function on_submit(obj)
{
	obj.isclick = 1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
	return true;
}

function loadFileList() {
	var folderSelect = document.getElementById("folderSelect");
	var fileList = document.getElementById("fileList");
	if (folderSelect.value === "ram") {
		<% FileList("ram"); %>
	} else if (folderSelect.value === "flash") {
		<% FileList("flash"); %>
	}
}


function loadFileContent() {
	var selectedFile = document.getElementById("fileList").value;
	var folderSelect = document.getElementById("folderSelect").value;
	var fullPath = "";

	if (folderSelect === "ram") {
		//fullPath = "/var/log/" + selectedFile;
		fullPath = "ram_log/" + selectedFile;
	} else if (folderSelect === "flash") {
		//fullPath = "/var/config/log/" + selectedFile;
		fullPath = "flash_log/" + selectedFile;
	}

	fetch(fullPath)
		.then(response => {
			if(!response.ok){
				throw new Error('Network response was not ok');
			}
			return response.text();
		})
		.then(content => {
				document.getElementById("fileContent").value = content;
				})
		.catch(error => {
			document.getElementById("fileContent").value = " ";
			});
}

function refreshClick()
{
	parent.document.getElementById("시스템 로그").click(); 
}
</script>
</head>

<body>
<div class="intro_main ">
	<p class="intro_title"><% multilang(LANG_SYSTEM_LOG); %></p>
</div>

<form action=/boaform/admin/formSysLog method=POST name=formSysLog id=formSysLog>
<div class="data_common data_common_notitle">
	<table>
		<tr>
			<th width=30%><% multilang(LANG_SYSTEM_LOG); %>:&nbsp;</th>
			<td>
				<input type="radio" value="0" name="logcap" onClick='changelogstatus()' <% checkWrite("log-cap0"); %>><% multilang(LANG_DISABLE); %>&nbsp;&nbsp;
				<input type="radio" value="1" name="logcap" onClick='changelogstatus()' <% checkWrite("log-cap1"); %>><% multilang(LANG_ENABLE); %>
			</td>
		</tr>    
		<% ShowPPPSyslog("syslogppp"); %>		
		<TBODY id='sysgroup'>
			<tr>
				<th><% multilang(LANG_LOG_LEVEL); %>:&nbsp;</th>
				<td><select name='levelLog' size="1">
					<% checkWrite("syslog-log"); %>
				</select></td>
			</tr>
			<tr>
				<th><% multilang(LANG_REMOTE_LOG_LEVEL); %>:&nbsp;</th>
				<td ><select name='levelRemote' size="1">
					<% checkWrite("syslog-remote"); %>
				</select></td>
			</tr>
			<% RemoteSyslog("syslog-line"); %>
			<% RemoteSyslog("syslog-mode"); %>
			<tbody id='srvInfo'>
				<% RemoteSyslog("server-info"); %>
			</tbody>
		</TBODY>
	</table>
</div>

<div class="btn_ctl">
	<input class="link_bg" type="submit" value="<% multilang(LANG_APPLY_CHANGES); %>" name="apply" onClick="return saveClick(this)">
</div>
<div class="data_common data_common_notitle">
	<table>
		<tr>
			<th width=30%><% multilang(LANG_SAVE_LOG_TO_FILE); %>:</th>
			<td><input class="inner_btn" type="submit" value="<% multilang(LANG_SAVE); %>..." name="save_log" onClick="return on_submit(this)"></td>
		</tr>
		<tr>
			<th><% multilang(LANG_CLEAR_LOG); %>:</th>
			<td><input class="inner_btn" type="submit" value="<% multilang(LANG_RESET); %>" name="clear_log" onClick="return on_submit(this)"></td>
		</tr>
	</table>
</div>
<div class="column">
	<div class="column_title">
		<div class="column_title_left"></div>
			<p><% multilang(LANG_SYSTEM_LOG); %></p>
		<div class="column_title_right"></div>
	</div>
	<div class="data_common">
		<table>
			<% sysLogList(); %>
			<tr>
				<th width=30%><% multilang(LANG_SYSTEM_LOG_TYPE); %>:&nbsp;</th>
				<td>
					<select id="folderSelect" name="folder_Select" onChange="loadFileList()">
						<option value="ram" selected>RAM</option>
						<option value="flash">Flash</option>
					</select>
				</td>
			</tr> 
			<tr>
				<th width=30%><% multilang(LANG_SYSTEM_LOG_LIST); %>:&nbsp;</th>
				<td>
					<select id="fileList" name="file_List">
						<option value="" disabled>Select a file</option>
					</select>
				</td>
			</tr> 
            <tr>
                <th><% multilang(LANG_SYSTEM_LOG_PRINT); %>:</th>
                <td><input class="inner_btn" type="button" value="<% multilang(LANG_APPLY); %>" name="testprint_log" onClick="loadFileContent()"></td>
            </tr>
			<tr>
				<td colspan="2">
				<textarea id="fileContent" style='color:#ffffff; background-color:black; width:100%; height:300px; resize: none; min-height: 400px; overflow: auto; white-space: pre;' readonly></textarea>

				</td>
			</tr>
		</table>
		
	</div>
</div>

<div class="btn_ctl">

	 <input class="link_bg" type="button" value="새로고침" name="refresh" onClick="refreshClick()">
	<input type="hidden" value="/syslog.asp" name="submit-url">
	<input type="hidden" name="postSecurityFlag" value="">
</div>
<script>
	check_enable();
	//scrollElementToEnd(this.formSysLog.msg);
</script>
</form>
<script>

	<% initPage("syslog"); %>
	<% initPage("pppSyslog"); %>
	<% FileList("ram"); %>

	loadFileContent();

</script>
</blockquote>
</body>
</html>


