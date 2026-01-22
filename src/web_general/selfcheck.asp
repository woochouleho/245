<% SendWebHeadStr();%>
<title><% multilang(LANG_SELF_CHECK); %></title>

<script>
var user_level = 0;
<% user_confirm(); %>

function goClick()
{
    var submit_elm=document.getElementById("pingSubmit");

    if (document.ping.pingAddr.value=="") {
        alert("Enter host address !");
        document.ping.pingAddr.focus();
        return false;
    }

    document.getElementById("status").style.display = "";
    if (submit_elm.value=="Start")
        document.ping.pingAct.value="Start";
    else
        document.ping.pingAct.value="Stop";
    document.getElementById("pingSubmit").value = "Stop"
        postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
}

function mld_goClick()
{
    var submit_elm=document.getElementById("mldtableSubmit");

	if (user_level != 1) {
		alert("Not support account level");
		return false;
	}

    document.getElementById("igmpstatus").style.display = "";
    if (submit_elm.value=="Search")
        document.grouptable.grouptableAct.value="mldSearch";
    else
        document.grouptable.grouptableAct.value="mldReset";

    document.getElementById("mldtableSubmit").value = "Reset"
		postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
}

function igmp_goClick()
{
    var submit_elm=document.getElementById("grouptableSubmit");

	if (user_level != 1) {
		alert("Not support account level");
		return false;
	}

    document.getElementById("igmpstatus").style.display = "";
    if (submit_elm.value=="Search")
        document.grouptable.grouptableAct.value="igmpSearch";
    else
        document.grouptable.grouptableAct.value="igmpReset";

    document.getElementById("grouptableSubmit").value = "Reset"
		postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
}

function ping_goClick()
{
	var submit_elm=document.getElementById("pingSubmit");

    if (isAttackXSS(document.ping.pingAddr.value)) {
        alert("ERROR: Please check the value again");
        document.ping.pingAddr.focus();
        return false;
    }

    if (isValidIpAddressValue(document.ping.pingAddr.value)) {
        alert("ERROR: Please check the value again");
        document.ping.pingAddr.focus();
        return false;
    }

    if (isAttackXSSCmdInjection(document.ping.pingAddr.value)) {
        alert("ERROR: Please check the value again");
        document.ping.pingAddr.focus();
        return false;
    }

	if (document.ping.pingAddr.value=="") {
		alert("Enter host address !");
		document.ping.pingAddr.focus();
		return false;
	}

	document.getElementById("status").style.display = "";
	if (submit_elm.value=="Start")
		document.ping.pingAct.value="Start";
	else
		document.ping.pingAct.value="Stop";
	document.getElementById("pingSubmit").value = "Stop"
		postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
}

function net_goClick()
{
	var submit_elm=document.getElementById("netstatSubmit");

	document.getElementById("netstatus").style.display = "";
	if (submit_elm.value=="Search")
		document.netstat.netstatAct.value="Search";
	else
		document.netstat.netstatAct.value="Reset";

	document.getElementById("netstatSubmit").value = "Reset"
		postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
}

function on_init(){
	<% clearPingResult(); %>
	<% clearNetstatResult(); %>
	<% clearGroupTableResult(); %>
}
</script>
</head>

<body onload="on_init();">
<div class="intro_main ">
<p class="intro_title"><% multilang(LANG_SELF_CHECK); %></p>
<p class="intro_content"></p>
</div>
<form action=/boaform/admin/formSelfCheck method=POST name="formSelfCheck">

<div class="column_title">
	<div class="column_title_left"></div>
		<p><% multilang(LANG_LAN_SELF_CHECK); %></p>
		<p class="intro_content"></p>
	<div class="column_title_right"></div>
</div>
<div class="data_common ">
    <table>
        <tr>
            <th width=40%>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;LAN1</th>
            <td width=60%><% ShowPortStatus("lan1"); %></td>
        </tr>
        <tr>
            <th width=40%>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;LAN2</th>
            <td width=60%><% ShowPortStatus("lan2"); %></td>
        </tr>
        <tr>
            <th width=40%>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;LAN3</th>
            <td width=60%><% ShowPortStatus("lan3"); %></td>
        </tr>
        <tr>
            <th width=40%>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;LAN4</th>
            <td width=60%><% ShowPortStatus("lan4"); %></td>
        </tr>
    </table>
	</div>
</div>

<div class="column_title">
	<div class="column_title_left"></div>
		<p><% multilang(LANG_DHCP_SELF_CHECK); %></p>
		<p class="intro_content"></p>
	<div class="column_title_right"></div>
</div>
<div class="data_common ">
    <table>
        <tr>
            <th width=40%>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;WAN IP 할당</th>
            <td width=60%><% ShowDaemonCheckStatus("udhcpc"); %></td>
        </tr>
        <tr>
            <th width=40%>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;LAN IP 할당</th>
            <td width=60%><% ShowDaemonCheckStatus("udhcpd"); %></td>
        </tr>
    </table>
</div>

<div class="column_title">
	<div class="column_title_left"></div>
		<p><% multilang(LANG_WAN_DNS_SELF_CHECK); %></p>
		<p class="intro_content"></p>
	<div class="column_title_right"></div>
</div>
	<div class="data_common ">
    <table>
        <tr>
            <th width=40%>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;게이트웨이</th>
            <td width=60%><% ShowGatewayDnsCheckStatus("gatewaycheck"); %></td>
        </tr>
        <tr>
            <th width=40%>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;DNS</th>
            <td width=60%><% ShowGatewayDnsCheckStatus("dnscheck"); %></td>
        </tr>
    </table>
</div>

<div class="column_title">
	<div class="column_title_left"></div>
		<p><% multilang(LANG_IGMP_SELF_CHECK); %></p>
		<p class="intro_content"></p>
	<div class="column_title_right"></div>
</div>
<div class="data_common ">
    <table>
        <tr>
            <th width="40%">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;IGMP 상태</th>
            <td width="60%"><% ShowDaemonCheckStatus("igmpproxy"); %></td>
        </tr>
    </table>
</div>
<div class="btn_ctl">
  <input type="hidden" value="/admin/selfcheck.asp" name="submit-url">
  <input type="hidden" name="postSecurityFlag" value="">
</div>
</form>

<form action=/boaform/formGroupTable method=POST name="grouptable">
<div class="column_title">
    <div class="column_title_left"></div>
        <p><% multilang(LANG_IGMP_SELF_TABLE); %></p>
        <p class="intro_content"></p>
    <div class="column_title_right"></div>
</div>
<div class="data_common ">
    <table>
     <tr>
       <th width="40%">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;IGMP Table</th>
       <td width="60%"><input id="grouptableSubmit" class="link_bg" type="submit" value="Search" onClick="return igmp_goClick()"></td>
     </tr>
     <tr>
       <th width="40%">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;MLD Table</th>
       <td width="60%"><input id="mldtableSubmit" class="link_bg" type="submit" value="Search" onClick="return mld_goClick()"> </td>
     </tr>
    </table>
</div>
<div class="data_common data_common_notitle" id="igmpstatus" style="display:none">
    <iframe src="/admin/grouptable_result.asp" width="100%" height="100%"></iframe>
</div>
<div class="btn_ctl">
  <input type="hidden" name="grouptableAct">
  <input type="hidden" value="/admin/selfcheck.asp" name="submit-url">
</div>
</form>

<form action=/boaform/admin/formPing method=POST name="ping">
<div class="column_title">
    <div class="column_title_left"></div>
        <p><% multilang(LANG_PING_SELF_CHECK); %></p>
        <p class="intro_content"></p>
    <div class="column_title_right"></div>
</div>
<div class="data_common ">
    <table>
      <tr>
          <th width="40%"><% multilang(LANG_HOST_ADDRESS); %></th>
          <td width="30%"><input type="text" name="pingAddr" size="30" maxlength="30"></td>
		  <td width="30%"><input id="pingSubmit" class="link_bg" type="submit" value="Start" onClick="return ping_goClick()"></td>
      </tr>
    </table>
</div>
<div class="data_common data_common_notitle" id="status" style="display:none">
    <iframe src="/admin/ping_result.asp" width="100%" height="100%"></iframe>
</div>
<div class="btn_ctl">
      <input type="hidden" name="pingAct">
  	  <input type="hidden" value="/admin/selfcheck.asp" name="submit-url">
</div>
</form>

<form action=/boaform/admin/formNetstat method=POST name="netstat">
<div class="column_title">
	<div class="column_title_left"></div>
		<p></p>
		<p class="intro_content"></p>
	<div class="column_title_right"></div>
</div>
	<div class="data_common ">
    <table>
        <tr>
            <th width=40%>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;접속 포트 조회</th>
			<td width=60%><input id="netstatSubmit" class="link_bg" type="submit" value="Search" onClick="return net_goClick()"></td>
        </tr>
    </table>
    </div> 

<div class="data_common data_common_notitle" id="netstatus" style="display:none">
    <iframe src="/admin/netstat_result.asp" width="100%" height="100%"></iframe>
</div>

	<div class="column">
		<div class="column_title">
			<div class="column_title_left"></div>
				<p><% multilang(LANG_NEIGHBORLIST); %></p>
			<div class="column_title_right"></div>
		</div>
		<div class="data_common data_vertical">
		<table>
			<% neighborList(); %>
		</table>
		</div>
	</div>  
<div class="btn_ctl">
      <input type="hidden" name="netstatAct">
  	  <input type="hidden" value="/admin/selfcheck.asp" name="submit-url">
</div>
</form>

</body>
</html>
