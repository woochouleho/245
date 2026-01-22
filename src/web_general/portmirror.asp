<% SendWebHeadStr();%>
<title>Port Mirror <% multilang(LANG_DIAGNOSTICS); %></title>

<SCRIPT>
var mingport = 0;
var medport = 0;
var rxtx = 0;
<% portmirrorinit(); %>

function on_init()
{
	if (mingport == 1)
		document.portmirror.mingport0.checked = true;
	else if (mingport == 2)
		document.portmirror.mingport1.checked = true;
	else if (mingport == 3)
		document.portmirror.mingport2.checked = true;
	else if (mingport == 4)
		document.portmirror.mingport3.checked = true;
	else if (mingport == 5)
		document.portmirror.mingport4.checked = true;

	if (rxtx == 1)
		document.portmirror.rxtx0.checked = true;
	else if (rxtx == 2)
		document.portmirror.rxtx1.checked = true;
	else if (rxtx == 3)
		document.portmirror.rxtx2.checked = true;

	if (medport == 1)
		document.portmirror.medport1.checked = true;
	else if (medport == 2)
		document.portmirror.medport2.checked = true;
	else if (medport == 3)
		document.portmirror.medport3.checked = true;
	else if (medport == 4)
		document.portmirror.medport4.checked = true;
}

function goClick()
{
	var submit_elm=document.getElementById("portmirrorSubmit");
	var mingport_cnt = 0;
	var medport_cnt = 0;
	var rxtx_cnt = 0;

	if ((document.portmirror.mingport0.checked == false) &&
		(document.portmirror.mingport1.checked == false) &&
		(document.portmirror.mingport2.checked == false) &&
		(document.portmirror.mingport3.checked == false) &&
		(document.portmirror.mingport4.checked == false)) {
		// for initial
		// alert("Enter Port Mirroring mingPort Select!");
		// return false;
	} else {
		if (document.portmirror.mingport0.checked == true)
			mingport_cnt++;
		if (document.portmirror.mingport1.checked == true)
			mingport_cnt++;
		if (document.portmirror.mingport2.checked == true)
			mingport_cnt++;
		if (document.portmirror.mingport3.checked == true)
			mingport_cnt++;
		if (document.portmirror.mingport4.checked == true)
			mingport_cnt++;

		if (mingport_cnt == 0) {
			alert("Mirroring port is more than one select!");
			return false;
		} else if (mingport_cnt > 1) {
			alert("Mirroring max port is two port!");
			return false;
		}
	}

	if ((document.portmirror.medport1.checked == false) &&
		(document.portmirror.medport2.checked == false) &&
		(document.portmirror.medport3.checked == false) &&
		(document.portmirror.medport4.checked == false)) {
		// for initial
		// alert("Enter Port Mirrored medPort Select!");
		// return false;
	} else {
		if (document.portmirror.medport1.checked == true)
			medport_cnt++;
		else if (document.portmirror.medport2.checked == true)
			medport_cnt++;
		else if (document.portmirror.medport3.checked == true)
			medport_cnt++;
		else if (document.portmirror.medport4.checked == true)
			medport_cnt++;

		if (medport_cnt > 1) {
			alert("Mirrored port is only one select!");
			return false;
		}
	}

	if (document.portmirror.mingport1.checked == true && 
			document.portmirror.medport1.checked == true) {
		alert("Can't be set at the same port");
		return false;
	}

	if (document.portmirror.mingport2.checked == true && 
			document.portmirror.medport2.checked == true) {
		alert("Can't be set at the same port");
		return false;
	}

	if (document.portmirror.mingport3.checked == true && 
			document.portmirror.medport3.checked == true) {
		alert("Can't be set at the same port");
		return false;
	}

	if (document.portmirror.mingport4.checked == true && 
			document.portmirror.medport4.checked == true) {
		alert("Can't be set at the same port");
		return false;
	}

	if ((document.portmirror.rxtx0.checked == false) &&
		(document.portmirror.rxtx1.checked == false) &&
		(document.portmirror.rxtx2.checked == false)) {
		// for initial
		// alert("Enter Port Mirrored rxtxPort Select!");
		// return false;
	} else {
		if (document.portmirror.rxtx0.checked == true)
			rxtx_cnt++;
		else if (document.portmirror.rxtx1.checked == true)
			rxtx_cnt++;
		else if (document.portmirror.rxtx2.checked == true)
			rxtx_cnt++;

		if (rxtx_cnt > 1) {
			alert("Action is only one select!");
			return false;
		}
	}

	document.getElementById("status").style.display = "";
	if (submit_elm.value=="Apply")
		document.portmirror.portmirrorAct.value="Apply";
	else
		document.portmirror.portmirrorAct.value="Stop";

	document.portmirror.portmirrorAct.value="Stop";
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
}

</SCRIPT>
</head>

<body>
<div class="intro_main ">
	<p class="intro_title">Port Mirror <% multilang(LANG_DIAGNOSTICS); %></p>
</div>

<form action=/boaform/formPortMirror method=POST name="portmirror">
<div class="data_common data_common_notitle">
	<table>
	<tr>
		<th width="30%"> From Port: </th>
		<td width="70%">
			<input type='checkbox' name='mingport0' value=1> WAN &nbsp;
			<input type='checkbox' name='mingport1' value=2> LAN1 &nbsp;
			<input type='checkbox' name='mingport2' value=3> LAN2 &nbsp;
			<input type='checkbox' name='mingport3' value=4> LAN3 &nbsp;
			<input type='checkbox' name='mingport4' value=5> LAN4 &nbsp;
		</td>
	</tr>
	<tr>
		<th width="30%"> To Port: </th>
		<td width="70%">
			<input type='checkbox' name='medport1' value=1> LAN1 &nbsp;
			<input type='checkbox' name='medport2' value=2> LAN2 &nbsp;
			<input type='checkbox' name='medport3' value=3> LAN3 &nbsp;
			<input type='checkbox' name='medport4' value=4> LAN4 &nbsp;
		</td>
	</tr>
	<tr>
		<th width="30%"> Direction: </th>
		<td width="70%">
			<input type='checkbox' name='rxtx0' value=1> RX &nbsp;
			<input type='checkbox' name='rxtx1' value=2> TX &nbsp;
			<input type='checkbox' name='rxtx2' value=3> RXTX &nbsp;
		</td>
	</tr>

	</table>
</div>

<div class="data_common data_common_notitle" id="status" style="display:none">
	<iframe src="portmirror_result.asp" width="100%" height="100%"></iframe>
</div>
<div class="btn_ctl">
      <input type="hidden" name="portmirrorAct">
      <input id="portmirrorSubmit" class="link_bg" type="submit" value="Apply" onClick="return goClick()">
      <input type="hidden" value="/portmirror.asp" name="submit-url">
      <input type="hidden" name="postSecurityFlag" value="">
</div>
<script>
	on_init();
</script>
</form>
<br><br>
</body>
</html>
