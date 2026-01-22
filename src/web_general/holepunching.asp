<%SendWebHeadStr(); %>
<title><% multilang(LANG_HOLEPUNCHING); %></title>

<script language="javascript">
var url = '<% getInfo("hole-server-url"); %>';
var port = '<% getInfo("hole-server-url-port"); %>';
var interval = '<% getInfo("hole-interval"); %>';

function getHolePort() {
	var val;

	val = parseInt(port);

	if (isNaN(val) || val == 0)
		val = 10219; // default system log line is 10219

	return val;
} 

function getHoleInterval() {
	var val;

	val = parseInt(interval);

	if (isNaN(val) || val == 0)
		val = 60; // default system log line is 60

	return val;
} 

function hideInfo(hide) {
	if (hide == 1) {
		document.forms[0].hole_server.value = '';
		document.forms[0].hole_server_port.value = '';
		document.forms[0].hole_interval.value = '';
	} else {
		document.forms[0].hole_server.value = url;
		document.forms[0].hole_server_port.value = getHolePort();
		document.forms[0].hole_interval.value = getHoleInterval(); 
	}
}

function hideholeInfo(hide) {
	var status = false;

	if (hide == 1) {
		status = true;
	}
	changeBlockState('holegroup', status);
}

function changeholestatus() {
	with (document.forms[0]) {
		if (holecap[1].checked) {
			hideholeInfo(0);
			hideInfo(0);
		} else {
			hideholeInfo(1);
			hideInfo(1);
		}
	}
}

function saveClick(obj)
{
	if (isAttackXSS(document.forms[0].hole_server_port.value)) {
		alert("ERROR: Please check the value again");
		document.forms[0].hole_server_port.focus();
		return false;
	}

	if (isAttackXSS(document.forms[0].hole_interval.value)) {
		alert("ERROR: Please check the value again");
		document.forms[0].hole_interval.focus();
		return false;
	}

	<% holePunghing("check-url"); %>
	<% holePunghing("check-port"); %>
	<% holePunghing("check-interval"); %>
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

</script>
</head>

<body>
<div class="intro_main ">
	<p class="intro_title"><% multilang(LANG_HOLEPUNCHING); %></p>
</div>

<form action=/boaform/admin/formHolepunching method=POST name=formHolepunching>
<div class="data_common data_common_notitle">
	<table>
		<tr>
			<th width=30%><% multilang(LANG_HOLEPUNCHING); %>:&nbsp;</th>
			<td>
				<input type="radio" value="0" name="holecap" onClick='changeholestatus()' <% checkWrite("hole-cap0"); %>><% multilang(LANG_DISABLE); %>&nbsp;&nbsp;
				<input type="radio" value="1" name="holecap" onClick='changeholestatus()' <% checkWrite("hole-cap1"); %>><% multilang(LANG_ENABLE); %>
			</td>
		</tr>    
		<TBODY id='holegroup'>
			<% holePunghing("hole-server"); %>
			<% holePunghing("hole-port"); %>
			<% holePunghing("hole-interval"); %>
		</TBODY>
	</table>
</div>

<div class="btn_ctl">
	<input class="link_bg" type="submit" value="<% multilang(LANG_APPLY_CHANGES); %>" name="apply" onClick="return saveClick(this)">
	<input type="hidden" value="/admin/holepunching.asp" name="submit-url">
	<input type="hidden" name="postSecurityFlag" value="">
</div>

<script>
	//scrollElementToEnd(this.formSysLog.msg);
</script>
</form>
<script>
	<% initPage("holepunching"); %>

</script>
</blockquote>
</body>
</html>


