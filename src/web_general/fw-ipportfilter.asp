<%SendWebHeadStr(); %>
<title>IP/Port <% multilang(LANG_FILTERING); %></title>

<script>
function skip () { this.blur(); }

function protocolSelection()
{
	if ( document.formFilterAdd.protocol.selectedIndex == 2 )
	{
		document.formFilterAdd.sfromPort.disabled = true;
		document.formFilterAdd.stoPort.disabled = true;
		document.formFilterAdd.dfromPort.disabled = true;
		document.formFilterAdd.dtoPort.disabled = true;
	}
	else
	{
		document.formFilterAdd.sfromPort.disabled = false;
		document.formFilterAdd.stoPort.disabled = false;
		document.formFilterAdd.dfromPort.disabled = false;
		document.formFilterAdd.dtoPort.disabled = false;
	}
}

function addClick(obj)
{
	if (document.formFilterAdd.ethertype.value == 0) {
		if (document.formFilterAdd.sip.value == "" && document.formFilterAdd.smask.value == ""
				&& document.formFilterAdd.dip.value == "" && document.formFilterAdd.dmask.value == ""
				&& document.formFilterAdd.sfromPort.value == "" && document.formFilterAdd.dfromPort.value == "") {		
			alert('<% multilang(LANG_FILTER_RULES_CAN_NOT_BE_EMPTY); %>');
			document.formFilterAdd.sip.focus();
			return false;
		}

		if ((document.formFilterAdd.sip.value == "" && document.formFilterAdd.smask.value != "")
				|| (document.formFilterAdd.dip.value == "" && document.formFilterAdd.dmask.value != "")) {		
			alert("IP address cannot be empty! It should be filled with 4 digit numbers as xxx.xxx.xxx.xxx.");
			document.formFilterAdd.sip.focus();
			return false;
		}

		if ((document.formFilterAdd.sip.value != "" && document.formFilterAdd.smask.value == "")
				|| (document.formFilterAdd.dip.value != "" && document.formFilterAdd.dmask.value == "")) {		
			alert("<% multilang(LANG_INVALID_IPV4_SUBNET_SHOULD_NOT_EMPTY); %>");
			document.formFilterAdd.sip.focus();
			return false;
		}

		if (document.formFilterAdd.sip.value!="") {
			if (!checkHostIP(document.formFilterAdd.sip, 0))
				return false;
			if ( document.formFilterAdd.smask.value != "" ) {
				if (!checkNetmask(document.formFilterAdd.smask, 0))
					return false;
			}
			if(!checkIPandMask(document.formFilterAdd.sip,document.formFilterAdd.smask))
				return false;
		}

		if (document.formFilterAdd.dip.value!="") {
			if (!checkHostIP(document.formFilterAdd.dip, 0))
				return false;
			if ( document.formFilterAdd.dmask.value != "" ) {
				if (!checkNetmask(document.formFilterAdd.dmask, 0))
					return false;
			}
			if(!checkIPandMask(document.formFilterAdd.dip,document.formFilterAdd.dmask))
				return false;

		}

		if ( document.formFilterAdd.sfromPort.value!="" ) {
			if ( validateKey( document.formFilterAdd.sfromPort.value ) == 0 ) {
				alert('<% multilang(LANG_INVALID_SOURCE_PORT); %>');
				document.formFilterAdd.sfromPort.focus();
				return false;
			}

			d1 = getDigit(document.formFilterAdd.sfromPort.value, 1);
			if (d1 > 65535 || d1 < 1) {
				alert('<% multilang(LANG_INVALID_SOURCE_PORT_NUMBER); %>');
				document.formFilterAdd.sfromPort.focus();
				return false;
			}

			if ( document.formFilterAdd.stoPort.value!="" ) {
				if ( validateKey( document.formFilterAdd.stoPort.value ) == 0 ) {
					alert('<% multilang(LANG_INVALID_SOURCE_PORT); %>');
					document.formFilterAdd.stoPort.focus();
					return false;
				}

				d1 = getDigit(document.formFilterAdd.stoPort.value, 1);
				if (d1 > 65535 || d1 < 1) {
					alert('<% multilang(LANG_INVALID_SOURCE_PORT_NUMBER); %>');
					document.formFilterAdd.stoPort.focus();
					return false;
				}
			}
		}

		if ( document.formFilterAdd.dfromPort.value!="" ) {
			if ( validateKey( document.formFilterAdd.dfromPort.value ) == 0 ) {
				alert('<% multilang(LANG_INVALID_DESTINATION_PORT); %>');
				document.formFilterAdd.dfromPort.focus();
				return false;
			}

			d1 = getDigit(document.formFilterAdd.dfromPort.value, 1);
			if (d1 > 65535 || d1 < 1) {
				alert('<% multilang(LANG_INVALID_DESTINATION_PORT_NUMBER); %>');
				document.formFilterAdd.dfromPort.focus();
				return false;
			}

			if ( document.formFilterAdd.dtoPort.value!="" ) {
				if ( validateKey( document.formFilterAdd.dtoPort.value ) == 0 ) {
					alert('<% multilang(LANG_INVALID_DESTINATION_PORT); %>');
					document.formFilterAdd.dtoPort.focus();
					return false;
				}

				d1 = getDigit(document.formFilterAdd.dtoPort.value, 1);
				if (d1 > 65535 || d1 < 1) {
					alert('<% multilang(LANG_INVALID_DESTINATION_PORT_NUMBER); %>');
					document.formFilterAdd.dtoPort.focus();
					return false;
				}
			}
		}
	}

	obj.isclick = 1;
	postTableEncrypt(document.formFilterAdd.postSecurityFlag, document.formFilterAdd);
	return true;
}

function disableDelButton()
{
  if (verifyBrowser() != "ns") {
	disableButton(document.formFilterDel.deleteSelFilterIpPort);
	disableButton(document.formFilterDel.deleteAllFilterIpPort);
  }
}

function on_submit(obj)
{
	obj.isclick = 1;
	postTableEncrypt(document.formFilterDefault.postSecurityFlag, document.formFilterDefault);
	return true;
}

function deleteClick(obj)
{
	if ( !confirm('<% multilang(LANG_CONFIRM_DELETE_ONE_ENTRY); %>') ) {
		return false;
	}
	else{
		obj.isclick = 1;
		postTableEncrypt(document.formFilterDel.postSecurityFlag, document.formFilterDel);
		return true;
	}
}
        
function deleteAllClick(obj)
{
	if ( !confirm('Do you really want to delete the all entries?') ) {
		return false;
	}
	else{
		obj.isclick = 1;
		postTableEncrypt(document.formFilterDel.postSecurityFlag, document.formFilterDel);
		return true;
	}
}
function dirChange()
{
	get_by_id("wanif_tr").style.display = "none";
	if (document.formFilterAdd.dir.value==1){
		get_by_id("wanif_tr").style.display = "";
	}
}

function ethertypeSelection()
{
	if (document.formFilterAdd.ethertype.value == 0) {
		get_by_id("proto_mode").style.display = "";
		get_by_id("source").style.display = "";
		get_by_id("dest").style.display = "";
	} else {
		get_by_id("proto_mode").style.display = "none";
		get_by_id("source").style.display = "none";
		get_by_id("dest").style.display = "none";
	}
}

</script>
</head>

<body>
<div class="intro_main ">
	<p class="intro_title">IP/Port <% multilang(LANG_FILTERING); %></p>
	<p class="intro_content"> <% multilang(LANG_PAGE_DESC_DATA_PACKET_FILTER_TABLE); %></p>
</div>

<form action=/boaform/formFilter method=POST name="formFilterDefault">
<div class="data_common data_common_notitle">
	<table>	
		<tr>
			<th width="30%"><% multilang(LANG_OUTGOING_DEFAULT_ACTION); %>:&nbsp;&nbsp;</th>
			<td width="70%"><input type="radio" name="outAct" value=0 <% checkWrite("ipf_out_act0"); %>><% multilang(LANG_DENY); %>&nbsp;&nbsp;
				<input type="radio" name="outAct" value=1 <% checkWrite("ipf_out_act1"); %>><% multilang(LANG_ALLOW); %>&nbsp;&nbsp;
			</td>
		</tr>
		<tr>
			<th width="30%"><% multilang(LANG_INCOMING_DEFAULT_ACTION); %>:&nbsp;&nbsp;</th>
			<td width="70%"><input type="radio" name="inAct" value=0 <% checkWrite("ipf_in_act0"); %>><% multilang(LANG_DENY); %>&nbsp;&nbsp;
				<input type="radio" name="inAct" value=1 <% checkWrite("ipf_in_act1"); %>><% multilang(LANG_ALLOW); %>&nbsp;&nbsp;
			</td>
		</tr>
	</table>
</div>
<div class="btn_ctl">
	<input class="link_bg" type="submit" value="<% multilang(LANG_APPLY_CHANGES); %>" name="setDefaultAction" onClick="return on_submit(this)">&nbsp;&nbsp;
	<input type="hidden" value="/fw-ipportfilter.asp" name="submit-url">
	<input type="hidden" name="postSecurityFlag" value="">
</div>
</form>

<form action=/boaform/formFilter method=POST name="formFilterAdd">
<div class="data_common data_common_notitle">
	<table>	
		<tr id="dir" style="display:on">
			<th width="30%">
				<% multilang(LANG_DIRECTION); %>: 
				<select name=dir onChange="dirChange()">
					<option select value=0><% multilang(LANG_OUTGOING); %></option>
					<option value=1><% multilang(LANG_INCOMING); %></option>
				</select>
			</th>
		</tr>
		<tr id="proto_mode" style="display:on">
			<th>
				<% multilang(LANG_PROTOCOL); %>: 
				<select name="protocol" onChange="protocolSelection()">
					<option select value=1>TCP</option>
					<option value=2>UDP</option>
					<option value=3>ICMP</option>
				</select>
			</th>
			<th>
				<% multilang(LANG_RULE_ACTION); %>:
		   		<input type="radio" name="filterMode" value="Deny" checked>&nbsp;<% multilang(LANG_DENY); %>
		   		<input type="radio" name="filterMode" value="Allow">&nbsp;&nbsp;<% multilang(LANG_ALLOW); %>
			</th>
		</tr>
		<tr id="etype" style="display:on">
			<th>
				<% multilang(LANG_ETHER_TYPE); %>: 
				<select name="ethertype" onChange="ethertypeSelection()">
					<option select value=0>-</option>
					<option value=1>IPV4</option>
					<option value=2>IPV6</option>
					<option value=3>ARP</option>
					<option value=4>PPPOE</option>
					<option value=5>WAKELAN</option>
					<option value=6>APPLETALK</option>
					<option value=7>PROFINET</option>
					<option value=8>FLOWCONTROL</option>
					<option value=9>IPX</option>
					<option value=10>MPLSUNI</option>
					<option value=11>MPLSMULTI</option>
					<option value=12>802.1X</option>
					<option value=13>LLDP</option>
					<option value=14>RARP</option>
					<option value=15>NETBIOS</option>
					<option value=16>X.25</option>
				</select>
			</th>
		</tr>
		<tr id="source" style="display:on">
			<th>
				<% multilang(LANG_SOURCE); %> <% multilang(LANG_IP_ADDRESS); %>:  <input type="text" name="sip" size="10" maxlength="15">
			</th>
			<th>
				<% multilang(LANG_SUBNET_MASK); %>:  <input type="text" name="smask" size="10" maxlength="15">
			</th>
			<th>
				<% multilang(LANG_PORT); %>: 
				<input type="text" name="sfromPort" size="4" maxlength="5">-
			 	<input type="text" name="stoPort" size="4" maxlength="5">&nbsp;&nbsp;
			</th>
		</tr>
		<tr id="dest" style="display:on">
			<th>
				<% multilang(LANG_DESTINATION); %> <% multilang(LANG_IP_ADDRESS); %>:  <input type="text" name="dip" size="10" maxlength="15">
			</th>
			<th>
				<% multilang(LANG_SUBNET_MASK); %>:  <input type="text" name="dmask" size="10" maxlength="15">
			</th>
			<th>
				<% multilang(LANG_PORT); %>: 
				<input type="text" name="dfromPort" size="4" maxlength="5">-
				<input type="text" name="dtoPort" size="4" maxlength="5">&nbsp;&nbsp;
			</th>
		</tr>
		<tr id="wanif_tr" style="display:none">
			<th width="30%">
				<% multilang(LANG_WAN_INTERFACE); %>: 
			</th>
			<td width="30%">
				<select name="wanif"><% if_wan_list("rt-any-vpn"); %></select>
			</td>
			<td></td>
		</tr>
	</table>
</div>
<div class="btn_ctl">
	<input class="link_bg" type="submit" value="<% multilang(LANG_ADD); %>" name="addFilterIpPort" onClick="return addClick(this)">
	<input type="hidden" value="/fw-ipportfilter.asp" name="submit-url">
	<input type="hidden" name="postSecurityFlag" value="">
</div>
</form>

<form action=/boaform/formFilter method=POST name="formFilterDel">
<div class="column">
	<div class="column_title">
		<div class="column_title_left"></div>
			<p><% multilang(LANG_CURRENT_FILTER_TABLE); %></p>
		<div class="column_title_right"></div>
	</div>
	<div class="data_common data_vertical">
		<table>
			<% ipPortFilterList(); %>
		</table>
	</div>
</div>
<div class="btn_ctl">
	<input class="link_bg" type="submit" value="<% multilang(LANG_DELETE_SELECTED); %>" name="deleteSelFilterIpPort" onClick="return deleteClick(this)">
	<input class="link_bg" type="submit" value="<% multilang(LANG_DELETE_ALL); %>" name="deleteAllFilterIpPort" onClick="return deleteAllClick(this)">
	<input type="hidden" value="/fw-ipportfilter.asp" name="submit-url">
	<input type="hidden" name="postSecurityFlag" value="">
</div>
<script>
	<% checkWrite("ipFilterNum"); %>
</script>
</form>
</body>
</html>
