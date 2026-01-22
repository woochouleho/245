<%SendWebHeadStr(); %>
<title><% getWanIfDisplay(); %> <% multilang(LANG_WAN); %></title>
<script type="text/javascript" src="base64_code.js"></script>
<script language="javascript">

var initConnectMode;
var pppConnectStatus=0;

var dgwstatus;
var gtwy;
var interfaceInfo = '';
var gtwyIfc ='';
var gwInterface=0;
var ipver=1;

var curlink = null;
var ctype = 4;
var temp_user_mvid_value = 0;
var sw_lan_port_num = <% checkWrite("sw_lan_port_num"); %>

var cgi = new Object();
var links = new Array();
var mape_routing_mode_support = <% checkWrite("mape_routing_mode_support"); %>;

with(links){<% initPageWaneth(); %>}

function show_password()
{
	var x= document.ethwan.pppPassword;

	if (x.type == "password") {
		x.type = "text";
	} else {
		x.type = "password";
	}
}

function isLocalWan()
{
	var ctype_value = document.ethwan.ctype.value;
	switch (ctype_value) {

		case '1':	// TR069
		case '8':	// VOICE
		case '9':	// TR069 & VOICE
			return 1;
		default:
			return 0;
	}	/* End switch */
}

function pppTypeSelection()
{
	if ( document.ethwan.pppConnectType.selectedIndex == 2) {
		document.ethwan.pppIdleTime.value = "";
		disableTextField(document.ethwan.pppIdleTime);
	}
	else {
		if (document.ethwan.pppConnectType.selectedIndex == 1) {
			enableTextField(document.ethwan.pppIdleTime);
		}
		else {
			document.ethwan.pppIdleTime.value = "";
			disableTextField(document.ethwan.pppIdleTime);
		}
	}
}

function checkDefaultGW() {
	with (document.forms[0]) {
		if (droute[0].checked == false && droute[1].checked == false && gwStr[0].checked == false && gwStr[1].checked == false) {			
			alert('<% multilang(LANG_A_DEFAULT_GATEWAY_HAS_TO_BE_SELECTED); %>');
			return false;
		}
		if (droute[1].checked == true) {
			if (gwStr[0].checked == true) {
				if (isValidIpAddress(dstGtwy.value, "Default Gateway IP Address") == false)
					return false;
			}
		}
	}
	return true;
}

function check_dhcp_opts()
{
	with (document.forms[0])
	{
		/* Option 60 */
		if(typeof enable_opt_60 !== 'undefined' &&enable_opt_60.checked)
		{
			if (opt60_val.value=="") {				
				alert('<% multilang(LANG_VENDOR_ID_CANNOT_BE_EMPTY); %>');
				opt60_val.focus();
				return false;
			}
			if (checkString(opt60_val.value) == 0) {				
				alert('<% multilang(LANG_INVALID_VENDOR_ID); %>');
				opt60_val.focus();
				return false;
			}
		}

		/* Option 61 */
		if(typeof enable_opt_61 !== 'undefined'&&enable_opt_61.checked)
		{
			if (iaid.value=="") {				
				alert('<% multilang(LANG_IAID_CANNOT_BE_EMPTY); %>');
				iaid.focus();
				return false;
			}
			if (checkDigit(iaid.value) == 0) {				
				alert('<% multilang(LANG_IAID_SHOULD_BE_A_NUMBER); %>');
				iaid.focus();
				return false;
			}

			if(duid_type[1].checked)
			{
				/* Enterprise number + Identifier*/
				if (duid_ent_num.value=="") {					
					alert('<% multilang(LANG_ENTERPRISE_NUMBER_CANNOT_BE_EMPTY); %>');
					duid_ent_num.focus();
					return false;
				}
				if (checkDigit(duid_ent_num.value) == 0) {					
					alert('<% multilang(LANG_ENTERPRISE_NUMBER_SHOULD_BE_A_NUMBER); %>');
					duid_ent_num.focus();
					return false;
				}

				if (duid_id.value=="") {
					alert('<% multilang(LANG_DUID_IDENTIFIER_CANNOT_BE_EMPTY); %>');
					duid_id.focus();
					return false;
				}
				if (checkString(duid_id.value) == 0) {					
					alert('<% multilang(LANG_INVALID_DUID_IDENTIFIER); %>');
					duid_id.focus();
					return false;
				}
			}
		}

		if(typeof enable_opt_125 !== 'undefined' &&enable_opt_125.checked)
		{
			if (manufacturer.value=="") {				
				alert('<% multilang(LANG_MANUFACTURER_OUI_CANNOT_BE_EMPTY); %>');
				manufacturer.focus();
				return false;
			}
			if (checkString(manufacturer.value) == 0) {				
				alert('<% multilang(LANG_INVALID_MANUFACTURER_OUI); %>');
				manufacturer.focus();
				return false;
			}

			if (product_class.value=="") {				
				alert('<% multilang(LANG_PRODUCT_CLASS_CANNOT_BE_EMPTY); %>');
				product_class.focus();
				return false;
			}
			if (checkString(product_class.value) == 0) {				
				alert('<% multilang(LANG_INVALID_PRODUCT_CLASS); %>');
				product_class.focus();
				return false;
			}

			if (model_name.value=="") {
				alert('<% multilang(LANG_MODEL_NAME_CANNOT_BE_EMPTY); %>');				
				model_name.focus();
				return false;
			}
			if (checkString(model_name.value) == 0) {				
				alert('<% multilang(LANG_INVALID_MODEL_NAME); %>');
				model_name.focus();
				return false;
			}

			if (serial_num.value=="") {				
				alert('<% multilang(LANG_SERIAL_NUMBER_CANNOT_BE_EMPTY); %>');
				serial_num.focus();
				return false;
			}
			if (checkString(serial_num.value) == 0) {				
				alert('<% multilang(LANG_SERIAL_NUMBER_CANNOT_BE_EMPTY); %>');
				serial_num.focus();
				return false;
			}
		}
	}
}

function disableUsernamePassword()
{
	//avoid sending username/password without encode
	disableTextField(document.ethwan.pppUserName);
	disableTextField(document.ethwan.pppPassword);
}
function applyCheck(obj)
{
	var tmplst = "";
	var ptmap = 0;
	var pmchkpt = document.getElementById("tbl_pmap");

	if (pmchkpt) {
		with (document.forms[0]) {
			if (<% checkWrite("wan_logic_port"); %>)
			{
				for (var i = 0; i < (sw_lan_port_num+10); i++) {
					/* chkpt do not always have 14 elements */
					if (!chkpt[i])
						break;

					if (chkpt[i].checked == true)
						ptmap |= (0x1 << i);
				}
			}
			else
			{
				for (var i = 0; i < 14; i++) {
					/* chkpt do not always have 14 elements */
					if (!chkpt[i])
						break;

					if (chkpt[i].checked == true)
						ptmap |= (0x1 << i);
				}
			}
			
			itfGroup.value = ptmap;
		}
	}

	if (checkDefaultGW()==false)
		return false;
	var str1 = document.ethwan.macclone.value;
	var str2 = document.ethwan.maccloneaddr.value;
	if( str1 != 0){
  		if ( str2.length != 12) {
			alert('<% multilang(LANG_INVALID_MAC_CLONE_ADDR_NOT_COMPLETE); %>');
			document.ethwan.maccloneaddr.focus();
			return false;
  		}

  		for (var i=0; i<str2.length; i++) {
    		if ( (str2.charAt(i) >= '0' && str2.charAt(i) <= '9') ||
				(str2.charAt(i) >= 'a' && str2.charAt(i) <= 'f') ||
				(str2.charAt(i) >= 'A' && str2.charAt(i) <= 'F') )
				continue;

			alert('<% multilang(LANG_INVALID_MAC_CLONE_ADDR_SHOULD_BE); %>');
			document.ethwan.maccloneaddr.focus();
			return false;
 	 	}
	}

	if (<% checkWrite("wan_logic_port"); %>)
	{
		if(logic_port.value == "")
		{
			alert("<% multilang(LANG_INCORRECT_WAN_PORT_NUM_SHOULE_NOT_BE_NULL); %>");
			return false;
		}
	}
	if (document.ethwan.vlan.checked == true) {
		if (document.ethwan.vid.value == "") {			
			alert('<% multilang(LANG_VID_SHOULD_NOT_BE_EMPTY); %>');
			document.ethwan.vid.focus();
			return false;
		}
		else if(document.ethwan.vid.value<0 ||document.ethwan.vid.value>4095) {
				alert("<% multilang(LANG_INCORRECT_VLAN_ID_SHOULE_BE_1_4095); %>");
				return false;
		}
		else if (!checkDigit(document.ethwan.vid.value)) {
			alert("ERROR: Please check the value");
			return false;
		}
	}
	
	/* Check if muticast vid is legal. */
	if (document.ethwan.multicast_vid.style.display != "none")
	{
		var multicast_vid_value = document.ethwan.multicast_vid.value
		/* Allow user can set vid = "", but we won't set value to mib. */
		if (multicast_vid_value.length != 0)
		{
			var multicast_vid_value = document.ethwan.multicast_vid.value
			if (multicast_vid_value < 1 || multicast_vid_value > 4095)
			{
				alert('<% multilang(LANG_MCAST_INVALID_VLAN_ID); %>');
				return false;
			} else if (!checkDigit(document.ethwan.multicast_vid.value)) {
				alert("ERROR: Please check the value");
				return false;
			}
		}
	}
	
	if ( document.ethwan.adslConnectionMode.value == 2 ) {
		if (document.ethwan.pppUserName.value=="") {			
			alert('<% multilang(LANG_PPP_USER_NAME_CANNOT_BE_EMPTY); %>');
			document.ethwan.pppUserName.focus();
			return false;
		}
		if (includeSpace(document.ethwan.pppUserName.value)) {			
			alert('<% multilang(LANG_CANNOT_ACCEPT_SPACE_CHARACTER_IN_PPP_USER_NAME); %>');
			document.ethwan.pppUserName.focus();
			return false;
		}
		if (checkString(document.ethwan.pppUserName.value) == 0) {			
			alert('<% multilang(LANG_INVALID_PPP_USER_NAME); %>');
			document.ethwan.pppUserName.focus();
			return false;
		}
		document.ethwan.encodePppUserName.value=encode64(document.ethwan.pppUserName.value);
		
		if (document.ethwan.pppPassword.value=="") {			
			alert('<% multilang(LANG_PPP_PASSWORD_CANNOT_BE_EMPTY); %>');
			document.ethwan.pppPassword.focus();
			return false;
		}

		if (includeSpace(document.ethwan.pppPassword.value)) {
			alert('<% multilang(LANG_CANNOT_ACCEPT_SPACE_CHARACTER_IN_PPP_PASSWORD); %>');
			document.ethwan.pppPassword.focus();
			return false;
		}
		if (checkString(document.ethwan.pppPassword.value) == 0) {
			alert('<% multilang(LANG_INVALID_PPP_PASSWORD); %>');
			document.ethwan.pppPassword.focus();
			return false;
		}
		document.ethwan.encodePppPassword.value=encode64(document.ethwan.pppPassword.value);
		document.ethwan.pppPassword.disabled=true;
		
		if (document.ethwan.pppConnectType.selectedIndex == 1) {
			if (document.ethwan.pppIdleTime.value <= 0) {				
				alert('<% multilang(LANG_INVALID_PPP_IDLE_TIME); %>');
				document.ethwan.pppIdleTime.focus();
				return false;
			}
		}
	}

	if (document.ethwan.dns1.value !="")
	{
		if (!checkHostIP(document.ethwan.dns1, 1))				
		{					
			document.ethwan.dns1.focus();
			return false;
		}
	}
	
	if (document.ethwan.dns2.value !="")
	{
		if (!checkHostIP(document.ethwan.dns2, 1))				
		{					
			document.ethwan.dns2.focus();
			return false;
		}
	}
	
	/*mtu*/
	if (document.getElementById("tbmtu").style.display != "none")
	{
		if( document.ethwan.mtu.value<1280 || document.ethwan.mtu.value>2000 )
		{
			alert("<% multilang(LANG_STRMRUERR); %>");
			document.ethwan.mtu.focus();
			return false;
		} else if (!checkDigit(document.ethwan.mtu.value)) {
			alert("ERROR: Please check the value");
			return false;
		}
	}

	if (document.getElementById("tbmtu_pppoe").style.display != "none")
	{
		if( document.ethwan.mtu_pppoe.value<1280 || document.ethwan.mtu_pppoe.value>1492 )
		{
			alert("<% multilang(LANG_STRMRUERR); %>");
			document.ethwan.mtu_pppoe.focus();
			return false;
		}
	}

	if (<% checkWrite("IPv6Show"); %>) {
		if(document.ethwan.IpProtocolType.value & 1){
			if ( document.ethwan.adslConnectionMode.value == 1 ) {
				if (document.ethwan.ipMode[0].checked)
				{
					/*Fixed IP*/
					if ( document.ethwan.ipUnnum.disabled || ( !document.ethwan.ipUnnum.disabled && !document.ethwan.ipUnnum.checked )) {
						if (!checkHostIP(document.ethwan.ip, 1))
							return false;
						if (document.ethwan.remoteIp.visiblity == "hidden") {
							if (!checkHostIP(document.ethwan.remoteIp, 1))
							return false;
						}
						if (document.ethwan.adslConnectionMode.value == 1 && !checkNetmask(document.ethwan.netmask, 1))
							return false;
					}
				}
				else
				{
					/* DHCP */
					if(check_dhcp_opts() == false)
						return false;
				}
			}
		}
	}

	if (<% checkWrite("IPv6Show"); %>) {
		/* Not bridged mode & choosing IPv6 */
		if (document.ethwan.adslConnectionMode.value != 0
			&& (document.ethwan.IpProtocolType.value != 1 || document.ethwan.v6TunnelType.value == 1 || document.ethwan.v6TunnelType.value == 2 || document.ethwan.v6TunnelType.value == 3) ) {

			if (document.ethwan.IpProtocolType.value & 2) {
				document.ethwan.v6TunnelType.value = 0;
			}

			document.ethwan.iana.disabled = false;	// those should be disable on wanAddrModeChange(), but we should submit value
			document.ethwan.iapd.disabled = false;	// those should be disable on wanAddrModeChange(), but we should submit value

			if(document.ethwan.AddrMode.value == '16') {
				if(document.ethwan.iana.checked == false && document.ethwan.iapd.checked == false ) {
					alert('<% multilang(LANG_PLEASE_SELECT_IANA_OR_IAPD); %>');
					document.ethwan.iana.focus();
					return false;
				}
			}

			if (document.ethwan.v6TunnelType.value == 2) {	// 6in4
				document.ethwan.Ipv6Addr.value = document.ethwan.Ipv6Addr6in4.value;
				document.ethwan.Ipv6PrefixLen.value = document.ethwan.Ipv6PrefixLen6in4.value;
				document.ethwan.Ipv6Gateway.value = document.ethwan.Ipv6Gateway6in4.value;
				document.ethwan.v6TunnelRv4IP.value = document.ethwan.v6TunnelRv4IP6in4.value;
				if ( document.ethwan.dnsV6Mode6in4[1].checked ) { //IPv6 DNS request is disable
					document.ethwan.dnsV6Mode[1].checked = true;
					document.ethwan.Ipv6Dns1.value = "";
					document.ethwan.Ipv6Dns2.value = "";
					document.ethwan.Ipv6Dns16rd.value = "";
					document.ethwan.Ipv6Dns26rd.value = "";
					document.ethwan.Ipv6Dns16to4.value = "";
					document.ethwan.Ipv6Dns26to4.value = "";
				}
				else {
					document.ethwan.dnsV6Mode[0].checked = true;
					document.ethwan.Ipv6Dns1.value = "";
					document.ethwan.Ipv6Dns2.value = "";
					document.ethwan.Ipv6Dns16rd.value = "";
					document.ethwan.Ipv6Dns26rd.value = "";
					document.ethwan.Ipv6Dns16in4.value = "";
					document.ethwan.Ipv6Dns26in4.value = "";
					document.ethwan.Ipv6Dns16to4.value = "";
					document.ethwan.Ipv6Dns26to4.value = "";
				}
			}

			if (document.ethwan.v6TunnelType.value == 3) {	// 6to4
				document.ethwan.v6TunnelRv4IP.value = document.ethwan.v6TunnelRv4IP6to4.value;
				document.ethwan.Ipv6Dns1.value = "";
				document.ethwan.Ipv6Dns2.value = "";
				document.ethwan.Ipv6Dns16rd.value = "";
				document.ethwan.Ipv6Dns26rd.value = "";
				document.ethwan.Ipv6Dns16in4.value = "";
				document.ethwan.Ipv6Dns26in4.value = "";
			}

			if(document.ethwan.AddrMode.value == '2') {
				if(document.ethwan.Ipv6Addr.value == "" || document.ethwan.Ipv6PrefixLen.value == "") {
					alert('<% multilang(LANG_PLEASE_INPUT_IPV6_ADDRESS_AND_PREFIX_LENGTH); %>');
					document.ethwan.Ipv6Addr.focus();
					return false;
				}
				if(document.ethwan.Ipv6Addr.value != ""){
					if (! isGlobalIpv6Address( document.ethwan.Ipv6Addr.value) ){
						alert('<% multilang(LANG_INVALID_IPV6_ADDRESS); %>');
						document.ethwan.Ipv6Addr.focus();
						return false;
					}
					var prefixlen= getDigit(document.ethwan.Ipv6PrefixLen.value, 1);
					if (prefixlen > 128 || prefixlen <= 0) {
						alert('<% multilang(LANG_INVALID_IPV6_PREFIX_LENGTH); %>');
						document.ethwan.Ipv6PrefixLen.focus();
						return false;
					}
				}
				if(document.ethwan.Ipv6Gateway.value != "" ){
					if (! isUnicastIpv6Address( document.ethwan.Ipv6Gateway.value) ){
						alert('<% multilang(LANG_INVALID_IPV6_GATEWAY_ADDRESS); %>');
						document.ethwan.Ipv6Gateway.focus();
						return false;
					}
				}
				if(document.ethwan.Ipv6Dns1.value != "" ){
					if (! isIpv6Address( document.ethwan.Ipv6Dns1.value) ){
						alert('<% multilang(LANG_INVALID_PRIMARY_IPV6_DNS_ADDRESS); %>');
						document.ethwan.Ipv6Dns1.focus();
						return false;
					}
				}
				if(document.ethwan.Ipv6Dns2.value != "" ){
					if (! isIpv6Address( document.ethwan.Ipv6Dns2.value) ){
						alert('<% multilang(LANG_INVALID_SECONDARY_IPV6_DNS_ADDRESS); %>');
						document.ethwan.Ipv6Dns2.focus();
						return false;
					}
				}
			}
			else if ( document.ethwan.dnsV6Mode[1].checked ) { //IPv6 DNS request is disable
				if(document.ethwan.Ipv6Dns1.value != "" ){
					if (! isUnicastIpv6Address( document.ethwan.Ipv6Dns1.value) ){
						alert('<% multilang(LANG_INVALID_PRIMARY_IPV6_DNS_ADDRESS); %>');
						document.ethwan.Ipv6Dns1.focus();
						return false;
					}
				}
				if(document.ethwan.Ipv6Dns2.value != "" ){
					if (! isUnicastIpv6Address( document.ethwan.Ipv6Dns2.value) ){
						alert('<% multilang(LANG_INVALID_SECONDARY_IPV6_DNS_ADDRESS); %>');
						document.ethwan.Ipv6Dns2.focus();
						return false;
					}
				}
			}

			//6RD
			if (<% checkWrite("6rdShow"); %>) {

				if (document.ethwan.v6TunnelType.value == 1) // 6rd(Manual)
				{
					if (document.ethwan.SixrdMode[1].checked) {
						if(document.ethwan.SixrdBRv4IP.value == ""){
							alert('<% multilang(LANG_INVALID_6RD_BOARD_ROUTER_V4IP_ADDRESS); %>');
							document.ethwan.SixrdBRv4IP.focus();
							return false;
						}

						if(document.ethwan.SixrdIPv4MaskLen.value == ""){
							alert('<% multilang(LANG_INVALID_6RD_IPV4_MASK_LENGTH); %>');
							document.ethwan.SixrdIPv4MaskLen.focus();
							return false;
						}

						if(document.ethwan.SixrdPrefix.value == ""){
							alert('<% multilang(LANG_INVALID_6RD_PREFIX_ADDRESS); %>');
							document.ethwan.SixrdPrefix.focus();
							return false;
						}

						if(document.ethwan.SixrdPrefixLen.value == ""){
							alert('<% multilang(LANG_INVALID_6RD_PREFIX_LENGTH); %>');
							document.ethwan.SixrdPrefixLen.focus();
							return false;
						}
					}

					if (document.ethwan.SixrdMode[0].checked || document.ethwan.SixrdMode[1].checked) {// 6rd(auto)
						if(document.ethwan.Ipv6Dns16rd.value != "" ){
							if (! isUnicastIpv6Address( document.ethwan.Ipv6Dns16rd.value) ){
								alert('<% multilang(LANG_INVALID_PRIMARY_IPV6_DNS_ADDRESS); %>');
								document.ethwan.Ipv6Dns16rd.focus();
								return false;
							}
						}
						if(document.ethwan.Ipv6Dns26rd.value != "" ){
							if (! isUnicastIpv6Address( document.ethwan.Ipv6Dns26rd.value) ){
								alert('<% multilang(LANG_INVALID_SECONDARY_IPV6_DNS_ADDRESS); %>');
								document.ethwan.Ipv6Dns26rd.focus();
								return false;
							}
						}
					}
				}
				else{

					document.ethwan.SixrdBRv4IP.value = "";
					document.ethwan.SixrdIPv4MaskLen.value = "";
					document.ethwan.SixrdPrefix.value = "";
					document.ethwan.SixrdPrefixLen.value = "";
					document.ethwan.Ipv6Dns16rd.value = "";
					document.ethwan.Ipv6Dns26rd.value = "";
				}

			}
		}
	}

	if(<% checkWrite("ConfigIPv6"); %>) {
		if (document.ethwan.IpProtocolType.value == 1 && (document.ethwan.v6TunnelType.value == 1 || document.ethwan.v6TunnelType.value == 2 || document.ethwan.v6TunnelType.value == 3)) {// 6rd or 6in4 or 6to4
			document.ethwan.iapd.checked = false;
		}
	}

	if(document.ethwan.lkname.value != "new") tmplst = curlink.name;
	document.ethwan.lst.value = tmplst;

	document.ethwan.qosEnabled.value = "ON"

	//avoid sending username/password without encode
	disableUsernamePassword();

	obj.isclick = 1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);

	return true;
}

function deleteCheck(obj)
{
	var tmplst = "";

	if ( document.ethwan.lkname.value == "new" )
	{		
		alert('<% multilang(LANG_NO_LINK_SELECTED); %>');
		return false;
	}

	tmplst = curlink.name;
	document.ethwan.lst.value = tmplst;

	disableUsernamePassword();

	obj.isclick = 1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);

	return true;
}

function setPPPConnected()
{
	pppConnectStatus = 1;
}

function dnsModeClicked()
{
	if ( document.ethwan.dnsMode[0].checked )
	{
		disableTextField(document.ethwan.dns1);
		disableTextField(document.ethwan.dns2);
	}
	
	if ( document.ethwan.dnsMode[1].checked )
	{
		document.ethwan.dns1.value = "210.220.163.82";
		disableTextField(document.ethwan.dns1);
		enableTextField(document.ethwan.dns2);
	}
}

function SixrdModeClicked()
{
	if (<% checkWrite("6rdShow"); %>) {
		if ( document.ethwan.SixrdMode[0].checked )
		{
			disableTextField(document.ethwan.SixrdBRv4IP);
			disableTextField(document.ethwan.SixrdIPv4MaskLen);
			disableTextField(document.ethwan.SixrdPrefix);
			disableTextField(document.ethwan.SixrdPrefixLen);
		}

		if ( document.ethwan.SixrdMode[1].checked )
		{
			enableTextField(document.ethwan.SixrdBRv4IP);
			enableTextField(document.ethwan.SixrdIPv4MaskLen);
			enableTextField(document.ethwan.SixrdPrefix);
			enableTextField(document.ethwan.SixrdPrefixLen);
		}
	}
}

function dnsModeV6Clicked()
{
	if ( document.ethwan.dnsV6Mode[0].checked )
	{
		disableTextField(document.ethwan.Ipv6Dns1);
		disableTextField(document.ethwan.Ipv6Dns2);
	}

	if ( document.ethwan.dnsV6Mode[1].checked )
	{
		enableTextField(document.ethwan.Ipv6Dns1);
		enableTextField(document.ethwan.Ipv6Dns2);
	}
}

function dnsModeV6Clicked6in4()
{
	if ( document.ethwan.dnsV6Mode6in4[0].checked )
	{
		disableTextField(document.ethwan.Ipv6Dns16in4);
		disableTextField(document.ethwan.Ipv6Dns26in4);
	}

	if ( document.ethwan.dnsV6Mode6in4[1].checked )
	{
		enableTextField(document.ethwan.Ipv6Dns16in4);
		enableTextField(document.ethwan.Ipv6Dns26in4);
	}
}

function disableFixedIpInput()
{
	disableTextField(document.ethwan.ip);
	disableTextField(document.ethwan.remoteIp);
	disableTextField(document.ethwan.netmask);
	
	document.ethwan.dnsMode[0].disabled = false;
	document.ethwan.dnsMode[1].disabled = false;	
	dnsModeClicked();

	if (<% checkWrite("6rdShow"); %>) {
		document.ethwan.SixrdMode[0].disabled = false;
		document.ethwan.SixrdMode[1].disabled = false;
		SixrdModeClicked();
	}
}

function enableFixedIpInput()
{
	enableTextField(document.ethwan.ip);
	enableTextField(document.ethwan.remoteIp);
	if (document.ethwan.adslConnectionMode.value == 4)
		disableTextField(document.ethwan.netmask);
	else
		enableTextField(document.ethwan.netmask);
	
	document.ethwan.dnsMode[0].disabled = true;
	document.ethwan.dnsMode[1].disabled = true;	
	dnsModeClicked();

	if (<% checkWrite("6rdShow"); %>) {
		document.ethwan.SixrdMode[0].disabled = true;
		document.ethwan.SixrdMode[1].disabled = true;
		SixrdModeClicked();
	}
}

function disableDNSv6Input()
{
	document.ethwan.dnsV6Mode[0].disabled = false;
	document.ethwan.dnsV6Mode[1].disabled = false;
	dnsModeV6Clicked();
}

function enableDNSv6Input()
{
	document.ethwan.dnsV6Mode[0].disabled = true;
	document.ethwan.dnsV6Mode[1].disabled = true;
	dnsModeV6Clicked();
}

function disableDNSv6Input6in4()
{
	document.ethwan.dnsV6Mode6in4[0].disabled = false;
	document.ethwan.dnsV6Mode6in4[1].disabled = false;
	dnsModeV6Clicked6in4();
}

function enableDNSv6Input6in4()
{
	document.ethwan.dnsV6Mode6in4[0].disabled = true;
	document.ethwan.dnsV6Mode6in4[1].disabled = true;
	dnsModeV6Clicked6in4();
}

function v6inv4TunnelSel()
{
	if(<% checkWrite("ConfigIPv6"); %>) {
		if (document.ethwan.IpProtocolType.value == 1 && document.ethwan.adslConnectionMode.value != 0) { // IPv4 only and not bridge
			if (<% checkWrite("6in4Tunnelshow"); %>) {
				document.getElementById('v6TunnelDiv').style.display='';
				v6TunnelChange();
			}
		}
		else {
			if (<% checkWrite("6in4Tunnelshow"); %>) {
				document.getElementById('v6TunnelDiv').style.display='none';
				document.getElementById('6in4Div').style.display='none';
				document.getElementById('6to4Div').style.display='none';
			}
			if (<% checkWrite("6rdShow"); %>) document.getElementById('6rdDiv').style.display='none';
		}
	}
}

function v6inv4Disable() {
	if(<% checkWrite("ConfigIPv6"); %>) {
		if (<% checkWrite("6in4Tunnelshow"); %>){
			document.getElementById('v6TunnelDiv').style.display='none';
			document.getElementById('6in4Div').style.display='none';
			document.getElementById('6to4Div').style.display='none';
		}
		if (<% checkWrite("6rdShow"); %>) document.getElementById('6rdDiv').style.display='none';
	}
}

function ipTypeSelection(init)
{
	if ( document.ethwan.ipMode[0].checked ) {
		enableFixedIpInput();
		showDhcpOptSettings(0);
	} else {
		disableFixedIpInput();
		showDhcpOptSettings(1);
	}
	
	if (init == 0) 
	{
		if ( document.ethwan.ipMode[0].checked )
			document.ethwan.dnsMode[1].checked = true;
		else
			document.ethwan.dnsMode[0].checked = true;
		dnsModeClicked();

		if (<% checkWrite("6rdShow"); %>) {
			if ( document.ethwan.ipMode[0].checked )
				document.ethwan.SixrdMode[1].checked = true;
			else
				document.ethwan.SixrdMode[0].checked = true;
			SixrdModeClicked();
		}
		v6inv4TunnelSel();
	}
}

function enable_pppObj()
{
	enableTextField(document.ethwan.pppUserName);
	enableTextField(document.ethwan.pppPassword);
	enableTextField(document.ethwan.pppConnectType);
	document.ethwan.gwStr[0].disabled = false;
	document.ethwan.gwStr[1].disabled = false;
	enableTextField(document.ethwan.dstGtwy);
	document.ethwan.wanIf.disabled = false;
	pppTypeSelection();
	autoDGWclicked();
}

function pppSettingsEnable()
{
	document.getElementById('tbl_ppp').style.display='block';
	v6inv4TunnelSel();
	enable_pppObj();
}

function disable_pppObj()
{
	disableTextField(document.ethwan.pppUserName);
	disableTextField(document.ethwan.pppPassword);
	disableTextField(document.ethwan.pppIdleTime);
	disableTextField(document.ethwan.pppConnectType);

	document.ethwan.gwStr[0].disabled = true;
	document.ethwan.gwStr[1].disabled = true;
	disableTextField(document.ethwan.dstGtwy);
	document.ethwan.wanIf.disabled = true;
}

function pppSettingsDisable()
{
	document.getElementById('tbl_ppp').style.display='none';
	v6inv4Disable();
	disable_pppObj();
}

function enable_ipObj()
{
	document.ethwan.ipMode[0].disabled = false;
	document.ethwan.ipMode[1].disabled = false;
	document.ethwan.gwStr[0].disabled = false;
	document.ethwan.gwStr[1].disabled = false;
	enableTextField(document.ethwan.dstGtwy);
	document.ethwan.wanIf.disabled = false;
	ipTypeSelection(1);
	autoDGWclicked();
}

function ipSettingsEnable()
{
	//if (ipver == 2)
	//	return;
	document.getElementById('tbl_ip').style.display='block';
	v6inv4TunnelSel();
	enable_ipObj();
}

function disable_ipObj()
{
	document.ethwan.ipMode[0].disabled = true;
	document.ethwan.ipMode[1].disabled = true;
	document.ethwan.gwStr[0].disabled = true;
	document.ethwan.gwStr[1].disabled = true;
	disableTextField(document.ethwan.dstGtwy);
	document.ethwan.wanIf.disabled = true;
	disableFixedIpInput();
}

function ipSettingsDisable()
{
	document.getElementById('tbl_ip').style.display='none';
	v6inv4Disable();
	showDhcpOptSettings(0);
	disable_ipObj();
}

function showDuidType2(show)
{
	if(show == 1)
	{
		document.getElementById('duid_t2_ent').style.display = '';
		document.getElementById('duid_t2_id').style.display = '';
	}
	else
	{
		document.getElementById('duid_t2_ent').style.display = 'none';
		document.getElementById('duid_t2_id').style.display = 'none';
	}
}

function showDhcpOptSettings(show)
{
	var dhcp_opt = document.getElementById('tbl_dhcp_opt');

	if(dhcp_opt == null)
		return ;

	if(show == 1)
	{
		document.getElementById('tbl_dhcp_opt').style.display='block';

		if(document.ethwan.duid_type[1].checked == true)
			showDuidType2(1);
		else
			showDuidType2(0);
	}
	else
		document.getElementById('tbl_dhcp_opt').style.display='none';
}

function ipModeSelection()
{
	if (document.ethwan.ipUnnum.checked) {
		disable_pppObj();
		disable_ipObj();
		document.ethwan.gwStr[0].disabled = false;
		document.ethwan.gwStr[1].disabled = false;
		enableTextField(document.ethwan.dstGtwy);
		document.ethwan.wanIf.disabled = false;
	}
	else
		enable_ipObj();
}

function updateBrMode(isLinkChanged)
{
	var brmode_ops = document.getElementById('brmode');

	if(!brmode_ops)
		return ;

	// reset to transparent bridge
	if(!isLinkChanged)
	{
		document.ethwan.br.checked = false;
		brmode_ops.value = 0;
		brmode_ops.disabled = true;
	}

	if(document.ethwan.adslConnectionMode.value == 0)
	{
		document.getElementById('br_row').style.display = "none";
		brmode_ops.disabled = false;
		if(brmode_ops.value == '') brmode_ops.selectedIndex = 0;
	}
	else
	{
		document.getElementById('br_row').style.display = "";
	}
}

function brClicked()
{
	var brmode_ops = document.getElementById('brmode');

	if(!brmode_ops)
		return ;

	if(document.ethwan.br.checked) {
		brmode_ops.disabled = false;
		if(brmode_ops.value == '') brmode_ops.selectedIndex = 0;
	} else
		brmode_ops.disabled = true;
}

/* This funcition is for creating and removing IPv6 Addrmode "Auto Detect Mode" option dynamically. */
function dynamic_create_ipv6_addrmode_options()
{
	var addrmode_options_len = document.getElementById("AddrMode").length;
	var addr_mode = document.ethwan.adslConnectionMode.value;
	var ipv6_auto_enable = <% checkWrite("ConfigIPv6_wan_auto_detect"); %>;
	var ipv6_auto_mode_option_name = "<% checkWrite("IPv6_auto_mode_string"); %>";

	if (ipv6_auto_enable == 1)
	{
		var get_ipv6_auto_option = 0;
		for(var i = 0; i < addrmode_options_len; i++)
		{
			if (document.getElementById("AddrMode").options[i].value == "32") /* Auto Detect Mode: 32 */
			{
				get_ipv6_auto_option = 1;
			}
		}

		switch (addr_mode) {
			case '0':	// bridge mode
				break;
			case '1':	// IPOE/1483 MER
			case '2':	// PPPoE
			case '3': 	// PPPoA
			case '4': 	// 1483 Routed
			case '5': 	// 1577 Routed
				if (get_ipv6_auto_option != 1)
				{
					/* Now we just put this option at last one. If you want to put it at other position,
					   you need to change below code. */
					document.getElementById("AddrMode").options[addrmode_options_len] = new Option(ipv6_auto_mode_option_name, 32);
				}
				break;
			case '8': 	// 6rd
				break;
			default:
				for(var i = 0; i < addrmode_options_len; i++)
				{
					if (document.getElementById("AddrMode").options[i].value == "32") /* Auto Detect Mode: 32 */
					{
						document.getElementById("AddrMode").remove(i);
					}
				}
				break;
		}	/* End switch */
	}
}

function wanLogicPortSelection()
{
	if (<% checkWrite("wan_logic_port"); %>)
	{
		var wan_port_mask = <% checkWrite("wan_port_mask"); %>
		
		
		for(var i=0; i< sw_lan_port_num; i++)
		{
			if(wan_port_mask & (0x01 << i))
			{		
				if((curlink != null)&&(i == curlink.logic_port)&&(document.ethwan.logic_port.value != curlink.logic_port)&&( curlink.logic_port_wan_num <= 1))
					document.ethwan.chkpt[i].disabled = false;
				else
					document.ethwan.chkpt[i].disabled = "disabled";
			}
			else
			{
				if(i == document.ethwan.logic_port.value)
					document.ethwan.chkpt[i].disabled = "disabled";
				else
					document.ethwan.chkpt[i].disabled = false;
			}			
		}
	
	}
}

function connectionTypeChangeDisplay()
{
	with (document.forms[0]) {
		if( adslConnectionMode.value!=0){
			document.getElementById('connectionType').style.display = "";
			document.getElementById('connectionTypeForBridge').style.display = 'none';
		}
		else{
			document.getElementById('connectionTypeForBridge').style.display = "";
			document.getElementById('connectionType').style.display = 'none';
		}
	}
}
function NATv6Change()
{
	if (document.getElementById('napt_v6').selectedIndex == 2)	{//nptv6 need ndp_proxy
		document.ethwan.ndp_proxy.disabled = true;
		document.ethwan.ndp_proxy.checked  = true;
	}else{
		document.ethwan.ndp_proxy.disabled = false;
		document.ethwan.ndp_proxy.checked  = false;
	}
}
function adslConnectionModeSelection(isLinkChanged)
{
	document.ethwan.naptEnabled.disabled = false;
	document.ethwan.igmpEnabled.disabled = false;
	document.ethwan.mldEnabled.disabled = false;
	document.ethwan.ipUnnum.disabled = true;

	document.ethwan.droute[0].disabled = false;
	document.ethwan.droute[1].disabled = false;
	if(!isLinkChanged)
		document.ethwan.mtu.value = 1500;
	document.getElementById('tbl_ppp').style.display = 'none';
	// document.getElementById('tbl_ip').style.display = 'none';

	if (document.getElementById('tbl_dhcp_opt') != null)
		document.getElementById('tbl_dhcp_opt').style.display = 'none';

	if (<% checkWrite("IPv6Show"); %>) {
		ipv6SettingsEnable((isLinkChanged==true)?0:1);
		document.getElementById('tbprotocol').style.display = "";
		document.ethwan.IpProtocolType.disabled = false;
	} else if (<% checkWrite("ConfigIPv6"); %>){
	        document.getElementById('tbprotocol').style.display = "none";
            document.ethwan.IpProtocolType.value = 1;
    }

	dynamic_create_ipv6_addrmode_options();
	switch (document.ethwan.adslConnectionMode.value) {
		case '0':// bridge mode
			if (<% checkWrite("ConfigIPv6"); %>)
			document.getElementById('tbmtu').style.display = 'none';
			document.getElementById('tbmtu_pppoe').style.display = 'none';
			document.ethwan.naptEnabled.disabled = true;
			document.ethwan.igmpEnabled.disabled = true;
			document.ethwan.mldEnabled.disabled = true;
			document.ethwan.droute[0].disabled = true;
			document.ethwan.droute[1].disabled = true;
			//document.ethwan.IpProtocolType.value = 2;     // IPV4/IPV6
			pppSettingsDisable();
			ipSettingsEnable();		// Bridge mode + WAN IP Settings
			// ipSettingsDisable(); // USE_DEONET, sghong, 20230706 
			protocolChange(0);

			// make checkbox 'qosEnalbed' able to be selected in Retailor wan page
			//if ( <% checkWrite("is_rtk_dev_ap"); %> ) document.ethwan.qosEnabled.disabled = true;
			break;
		case '1'://1483mer
			var tmp_ctype = document.ethwan.ctype.value;
			document.getElementById('tbmtu').style.display = '';
			document.getElementById('tbmtu_pppoe').style.display = 'none';
			pppSettingsDisable();
			if (<% checkWrite("IPv6Show"); %>) {
				if(document.ethwan.IpProtocolType.value != 2)		// It is not IPv6 only
					ipSettingsEnable();
			}
			else
				ipSettingsEnable();
			if(!isLinkChanged)
				document.ethwan.naptEnabled.checked = true;
			break;
		case '2'://PPPoE
			var tmp_ctype = document.ethwan.ctype.value;
			if(!isLinkChanged)
				document.ethwan.mtu_pppoe.value = 1492;
			document.getElementById('tbmtu_pppoe').style.display = '';
			document.getElementById('tbmtu').style.display = 'none';
			document.getElementById('tbl_ppp').style.display = 'block';
			ipSettingsDisable();
			pppSettingsEnable();
			if(!isLinkChanged)
				document.ethwan.naptEnabled.checked = true;

			if (<% checkWrite("6rdShow"); %>) {
				document.ethwan.SixrdMode[1].checked = true;	// 6rdMode is Manual
				document.ethwan.SixrdMode[0].disabled = true;
				document.ethwan.SixrdMode[1].disabled = true;
				SixrdModeClicked();
			}
			break;
		default:
			pppSettingsDisable();
			ipSettingsEnable();
	}

	updateBrMode(isLinkChanged);
	connectionTypeChangeDisplay();
}

function naptClicked()
{
	if (document.ethwan.adslConnectionMode.value == 3) {
		// Route1483
		if (document.ethwan.naptEnabled.checked == true) {
			document.ethwan.ipUnnum.checked = false;
			document.ethwan.ipUnnum.disabled = true;
		}
		else
			document.ethwan.ipUnnum.disabled = false;
		ipModeSelection();
	}
}

function vlanClicked()
{
	if (document.ethwan.vlan.checked)
	{
		document.ethwan.vid.disabled = false;
		document.ethwan.vprio.disabled = false;
	}
	else {
		document.ethwan.vid.disabled = true;
		document.ethwan.vprio.disabled = true;
	}
}

function hideGWInfo(hide) {
	var status = false;

	if (hide == 1)
		status = true;

	changeBlockState('gwInfo', status);

	if (hide == 0) {
		with (document.forms[0]) {
			if (dgwstatus == 255) {
				if (isValidIpAddress(gtwy) == true) {
					gwStr[0].checked = true;
					gwStr[1].checked = false;
					dstGtwy.value=gtwy;
					wanIf.disabled=true
				} else {
					gwStr[0].checked = false;
					gwStr[1].checked = true;
					dstGtwy.value = '';
				}
			}
			else if (dgwstatus != 239) {
					gwStr[1].checked = true;
					gwStr[0].checked = false;
					wanIf.disabled=false;
					wanIf.value=dgwstatus;
					dstGtwy.disabled=true;
			} else {
					gwStr[1].checked = false;
					gwStr[0].checked = true;
					wanIf.disabled=true;
					dstGtwy.disabled=false;
			}
		}
	}
}

function autoDGWclicked() {
	if (document.ethwan.droute[0].checked == true) {
		hideGWInfo(1);
	} else {
		hideGWInfo(0);
	}
}

function gwStrClick() {
	with (document.forms[0]) {
		if (gwStr[1].checked == true) {
			dstGtwy.disabled = true;
			wanIf.disabled = false;
		}
		else {
			dstGtwy.disabled = false;
			wanIf.disabled = true;
		}
      	}
}

function wanAddrModeChange(is_user)
{
	// initial page status
	document.getElementById('secIPv6Div').style.display="none";
	document.getElementById('dhcp6c_block').style.display="none";
	if (!isLocalWan()) //NOT TR069, VOIP and TR069_VOIP
	{
		document.ethwan.iana.disabled = false;
		document.ethwan.iapd.disabled = false;
	}else{
		document.ethwan.iana.disabled = true;
		document.ethwan.iapd.disabled = true;
	}

	if(is_user){
		// default Enable and ReadOnly IA_NA/IA_PD
		document.ethwan.iapd.checked = true;
		document.ethwan.iapd.checked = true;

		// default enable DNSv6 Request
		document.ethwan.dnsV6Mode[0].checked = true;
		document.ethwan.dnsV6Mode[1].checked = false;
		dnsModeV6Clicked();
	}

	if (document.ethwan.AddrMode.value == 0)
		document.ethwan.AddrMode.value = 1;

	switch(document.ethwan.AddrMode.value)
	{
		case '1':  //SLAAC
			document.getElementById('dhcp6c_block').style.display="";

			if(is_user){
				document.ethwan.iana.checked=false;
				if (isLocalWan()) //TR069, VOIP and TR069_VOIP
					document.ethwan.iapd.checked=false;
				else
					document.ethwan.iapd.checked=true;
			}
			break;
		case '2':	//Static
			document.getElementById('secIPv6Div').style.display='';

			if(is_user){
				document.ethwan.iana.checked=false;
				document.ethwan.iapd.checked=false;
			}
			break;
		case '16': //DHCP
			document.getElementById('dhcp6c_block').style.display='';

			if(is_user){
				document.ethwan.iana.checked=true;
				if (isLocalWan()) //TR069, VOIP and TR069_VOIP
					document.ethwan.iapd.checked=false;
				else
					document.ethwan.iapd.checked=true;
			}
			break;
		case '32':	//IPv6 WAN Auto Detect Mode
			document.getElementById('dhcp6c_block').style.display="";

			if(is_user){
				document.ethwan.iana.checked=true;
				if (isLocalWan()) //TR069, VOIP and TR069_VOIP
					document.ethwan.iapd.checked=false;
				else
					document.ethwan.iapd.checked=true;
			}
			break;
		default:
	}
}

function s4over6TypeChange()
{
	switch(document.ethwan.s4over6Type.value)
	{
		case '1': //dslite
			if(<% checkWrite("DSLiteShow"); %>){
				document.getElementById('DSLiteDiv').style.display="block";
				dsliteSettingChange();
			}
			break;
		case '2': //map-e
			if (<% checkWrite("MAPEShow"); %>){
				document.getElementById('mape_div').style.display="block";
				mapeSettingChange();
			}
			break;
		case '3': //map-t
			if (<% checkWrite("MAPTShow"); %>){
				document.getElementById('mapt_div').style.display="block";
				maptSettingChange();
			}
			break;
		case '4': //464xlat
			if (<% checkWrite("XLAT464Show"); %>){
				document.getElementById('XLAT464Div').style.display="block";
				xlat464SettingChange();
			}
			break;
		case '5': //lw4o6
			if (<% checkWrite("LW4O6Show"); %>){
				document.getElementById('LW4O6Div').style.display="block";
				lw4o6SettingChange();
			}
			break;
		default: //0
			if(<% checkWrite("DSLiteShow"); %>) document.getElementById('DSLiteDiv').style.display="none";
			if (<% checkWrite("MAPEShow"); %>) document.getElementById('mape_div').style.display="none";
			if (<% checkWrite("MAPTShow"); %>) document.getElementById('mapt_div').style.display="none";
			if (<% checkWrite("XLAT464Show"); %>) document.getElementById('XLAT464Div').style.display="none";
			if (<% checkWrite("LW4O6Show"); %>) document.getElementById('LW4O6Div').style.display="none";
	}
}

function ipv6WanUpdate(UseDefaultSetting)
{
	wanAddrModeChange(UseDefaultSetting);
}

function ipv6SettingsDisable()
{
	document.getElementById('tbipv6wan').style.display="none";
	document.getElementById('secIPv6Div').style.display="none";
	document.getElementById('dhcp6c_block').style.display="none";
	document.getElementById('IPv6DnsDiv').style.display="none";
	document.getElementById('tbnaptv6').style.display="none";
	document.getElementById('ndp_proxy').style.display="none";
	if(<% checkWrite("DSLiteShow"); %>) {
		document.getElementById('DSLiteDiv').style.display="none";
		document.getElementById('dslite_mode_div').style.display="none";
		document.getElementById('dslite_aftr_hostname_div').style.display="none";
	}
	
	//MAP-E
	if (<% checkWrite("MAPEShow"); %>){
		document.getElementById('mape_div').style.display="none";	
	}
	//MAP-T
	if (<% checkWrite("MAPTShow"); %>){
		document.getElementById('mapt_div').style.display="none";
	}
	//XLAT464
	if (<% checkWrite("XLAT464Show"); %>){
		document.getElementById('XLAT464Div').style.display="none";
	}
	//LW4O6
	if(<% checkWrite("LW4O6Show"); %>){
		document.getElementById('LW4O6Div').style.display="none";
	}
}

function ipv6SettingsEnable(UseDefaultSetting)
{
	if(document.ethwan.IpProtocolType.value != 1){
		document.getElementById('tbipv6wan').style.display="";
		document.getElementById('IPv6DnsDiv').style.display="";
		document.getElementById('TrIpv6AddrType').style.display="";
		if(<% checkWrite("napt_v6"); %>){
			document.getElementById('tbnaptv6').style.display="";
			NATv6Change();
		}
		if(<% checkWrite("ndp_proxy"); %>){
			document.getElementById('ndp_proxy').style.display="";
		}

		if (document.ethwan.IpProtocolType.value == 2) { // IPv6 only
			document.getElementById('div4over6').style.display="block";
			s4over6TypeChange();
		}
		else{
			document.getElementById('div4over6').style.display="none";
		}
		ipv6WanUpdate(UseDefaultSetting);
  	}
}

function dsliteSettingChange()
{
	with ( document.forms[0] ) 
	{
		if(document.ethwan.s4over6Type.value == 1){
			/*
			if (adslConnectionMode.value == 2) { //PPPoE
				if(mtu.value > 1400) //DS-lite must add IPv6 header for 40 bytes
					mtu.value = 1400;			
			}
			else {
				if(mtu.value > 1400)
					mtu.value = 1400;			
			}
			*/
			dslite_mode_div.style.display = '';
			dsliteAftrModeChange();
			//hide map-e
			if(<% checkWrite("MAPEShow"); %>){
				document.getElementById('mape_div').style.display="none";
			}
			//hide map-t
			if(<% checkWrite("MAPTShow"); %>){
				document.getElementById('mapt_div').style.display="none";
			}
			//hide xlat464
			if(<% checkWrite("XLAT464Show"); %>){
				document.getElementById('XLAT464Div').style.display="none";
			}
			//hide lw4o6
			if(<% checkWrite("LW4O6Show"); %>){
				document.getElementById('LW4O6Div').style.display="none";
			}
		}	
	}	
}

function dsliteAftrModeChange()
{

	with ( document.forms[0] ) 
	{
		document.getElementById('dslite_aftr_hostname_div').style.display="none";
		if (dslite_aftr_mode[1].checked == true) {
			document.getElementById('dslite_aftr_hostname_div').style.display="";
			if(dslite_aftr_hostname.value == "::") //clear the value
				dslite_aftr_hostname.value ="";
		}
	}

}

function xlat464SettingChange(){
	with( document.forms[0] )
	{
		if(document.ethwan.s4over6Type.value == 4){
			xlat464_clatmode_div.style.display="block";
			//xlat464 clat prefix6 mode: dhcpv6pd/static
			xlat464ClatPrefix6ModeChange();
			//display xlat464 plat prefix6
			document.getElementById('xlat464_platprefix6_div').style.display="block";

			//hide dslite
			if(<% checkWrite("DSLiteShow"); %>){
				document.getElementById('DSLiteDiv').style.display="none";
			}
			//hide map-e
			if(<% checkWrite("MAPEShow"); %>){
				document.getElementById('mape_div').style.display="none";
			}
			//hide map-t
			if(<% checkWrite("MAPTShow"); %>){
				document.getElementById('mapt_div').style.display="none";
			}
			//hide lw4o6
			if(<% checkWrite("LW4O6Show"); %>){
				document.getElementById('LW4O6Div').style.display="none";
			}
		}
	}
}

function xlat464ClatPrefix6ModeChange(){
	with( document.forms[0] )
	{
		document.getElementById('xlat464_clatprefix6_div').style.display="none";
		if(xlat464_plat_prefix6.value == "::") //clear the value
			xlat464_plat_prefix6.value ="";
		if(xlat464_clat_mode[1].checked){
			document.getElementById('xlat464_clatprefix6_div').style.display="block";
			if(xlat464_clat_prefix6.value == "::") //clear the value
				xlat464_clat_prefix6.value ="";
		}
	}
}

function lw4o6SettingChange(){
	with( document.forms[0] )
	{
		if(document.ethwan.s4over6Type.value == 5){
			lw4o6_mode_div.style.display="block";
			lw4o6ModeChange();

			//hide dslite
			if(<% checkWrite("DSLiteShow"); %>){
				document.getElementById('DSLiteDiv').style.display="none";
			}
			//hide map-e
			if(<% checkWrite("MAPEShow"); %>){
				document.getElementById('mape_div').style.display="none";
			}
			//hide map-t
			if(<% checkWrite("MAPTShow"); %>){
				document.getElementById('mapt_div').style.display="none";
			}
			//hide xlat464
			if(<% checkWrite("XLAT464Show"); %>){
				document.getElementById('XLAT464Div').style.display="none";
			}

		}
	}
}

function lw4o6ModeChange()
{
	with ( document.forms[0] )
	{
		document.getElementById('lw4o6_static_div').style.display="none";
		if (lw4o6_mode[1].checked == true) {
			document.getElementById('lw4o6_static_div').style.display="block";
			lw4o6_static_div.style.display = 'block';
			if(lw4o6_aftr_addr.value == "::") //clear the value
				lw4o6_aftr_addr.value ="";
			if(lw4o6_local_v6Prefix.value == "::")
				lw4o6_local_v6Prefix.value ="";
			if(lw4o6_local_v6PrefixLen.value == "0")
				lw4o6_local_v6PrefixLen.value ="";
			if(lw4o6_v4addr.value == "0.0.0.0")
				lw4o6_v4addr.value ="";
			if(lw4o6_psid_len.value == "0")
				lw4o6_psid_len.value ="";
			if(lw4o6_psid.value == "0x0")
				lw4o6_psid.value ="";
		}
	}
}

function v6TunnelChange()
{
	with ( document.forms[0] )
	{
		switch(document.ethwan.v6TunnelType.value)
		{
			case '1':  //6rd
				document.ethwan.AddrMode.value =1;   			// IPV6_WAN_AUTO
				document.ethwan.dnsV6Mode[1].checked = true;  	//IPv6 DNS request is disable
				if (<% checkWrite("6rdShow"); %>) document.getElementById('6rdDiv').style.display='';
				if (<% checkWrite("6in4Tunnelshow"); %>) {
					document.getElementById('6in4Div').style.display='none';
					document.getElementById('6to4Div').style.display='none';
				}
				break;
			case '2':	//6in4
				document.ethwan.AddrMode.value =2;						// IPV6_WAN_STATIC
				if (<% checkWrite("6rdShow"); %>) document.getElementById('6rdDiv').style.display='none'; // close 6rd setting page.
				if (<% checkWrite("6in4Tunnelshow"); %>) {
					document.getElementById('6in4Div').style.display='';
					document.getElementById('6to4Div').style.display='none';
					dnsModeV6Clicked6in4();
				}
				break;
			case '3':	//6to4
				document.ethwan.AddrMode.value =1;   		// IPV6_WAN_AUTO
				document.ethwan.dnsV6Mode[1].checked = true;  //IPv6 DNS request is disable
				if (<% checkWrite("6rdShow"); %>) document.getElementById('6rdDiv').style.display='none'; // close 6rd setting page.
				if (<% checkWrite("6in4Tunnelshow"); %>) {
					document.getElementById('6in4Div').style.display='none';
					document.getElementById('6to4Div').style.display='';
				}
				break;
			default:
				if (<% checkWrite("6rdShow"); %>) document.getElementById('6rdDiv').style.display='none'; // close 6rd setting page.
				if (<% checkWrite("6in4Tunnelshow"); %>) {
					document.getElementById('6in4Div').style.display='none';
					document.getElementById('6to4Div').style.display='none';
				}
				break;
		}
	}
}

//MAP-E Begin
function mapeSettingChange()
{
	with ( document.forms[0] ) 
	{
		if(document.ethwan.s4over6Type.value == 2){
			if (mape_routing_mode_support){
				mape_forward_mode_div.style.display = 'block';
			}
			mape_mode_div.style.display = 'block';
			mapeModeChange();

			//hide dslite
			if(<% checkWrite("DSLiteShow"); %>){
				document.getElementById('DSLiteDiv').style.display="none";
			}
			//hide MAP-T
			if(<% checkWrite("MAPTShow"); %>){
				document.getElementById('mapt_div').style.display="none";
			}
			//hide xlat464
			if(<% checkWrite("XLAT464Show"); %>){
				document.getElementById('XLAT464Div').style.display="none";
			}
			//hide lw4o6
			if(<% checkWrite("LW4O6Show"); %>){
				document.getElementById('LW4O6Div').style.display="none";
			}
		}	
	}
}

function mapeForwardModeChange(){
	with ( document.forms[0] ) 
	{
		if(mape_forward_mode[1].checked == true){//Routing
			document.ethwan.mape_forward_mode.value = "1";					
		}	
		else{//NAT
			document.ethwan.mape_forward_mode.value = "0";		
		}	
	}	
}

//MAP-T Begin
function maptSettingChange()
{
	with ( document.forms[0] )
	{
		if(document.ethwan.s4over6Type.value == 3){
			mapt_mode_div.style.display = 'block';
			maptModeChange();

			//hide dslite
			if(<% checkWrite("DSLiteShow"); %>){
				document.getElementById('DSLiteDiv').style.display="none";
			}
			//hide MAP-E
			if(<% checkWrite("MAPEShow"); %>){
				document.getElementById('mape_div').style.display="none";
			}
			//hide xlat464
			if(<% checkWrite("XLAT464Show"); %>){
				document.getElementById('XLAT464Div').style.display="none";
			}
			//hide lw4o6
			if(<% checkWrite("LW4O6Show"); %>){
				document.getElementById('LW4O6Div').style.display="none";
			}
		}
	}
}

function mapeModeChange()
{
	with ( document.forms[0] ) 
	{
		var mape_mode_val =mape_mode.value;		
		switch(mape_mode_val){
			case '0': //DHCP
				mape_static_div.style.display = 'none';
				//mape_fmr_tbl_div.style.display = "none";
				break;
			case '1': //Static
				mape_static_div.style.display = 'block';
				//mape_fmr_tbl_div.style.display = "block";
				if(mape_local_v6Prefix.value == "::") //clear the value
					mape_local_v6Prefix.value ="";
				if(mape_local_v6PrefixLen.value == "0")
					mape_local_v6PrefixLen.value ="";
				if(mape_remote_v6Addr.value == "::")
					mape_remote_v6Addr.value ="";
				if(mape_local_v4Addr.value == "0.0.0.0")
					mape_local_v4Addr.value ="";
				if(mape_local_v4MaskLen.value == "0")
					mape_local_v4MaskLen.value ="";
				if(mape_psid_offset.value == "0")
					mape_psid_offset.value ="";
				if(mape_psid_len.value == "0")
					mape_psid_len.value ="";
				if(mape_psid.value == "0x0")
					mape_psid.value ="";
				break;
			default:
				break;
		}
	}
}

//MAP-T
function maptModeChange()
{
	with ( document.forms[0] )
	{
		var mapt_mode_val =mapt_mode.value;
		switch(mapt_mode_val){
			case '0': //DHCP
				mapt_static_div.style.display = 'none';
				break;
			case '1': //Static
				mapt_static_div.style.display = 'block';
				if(mapt_local_v6Prefix.value == "::") //clear the value
					mapt_local_v6Prefix.value ="";
				if(mapt_local_v6PrefixLen.value == "0")
					mapt_local_v6PrefixLen.value ="";
				if(mapt_remote_v6Prefix.value == "::")
					mapt_remote_v6Prefix.value ="";
				if(mapt_remote_v6PrefixLen.value == "0")
					mapt_remote_v6PrefixLen.value ="";
				if(mapt_local_v4Addr.value == "0.0.0.0")
					mapt_local_v4Addr.value ="";
				if(mapt_local_v4MaskLen.value == "0")
					mapt_local_v4MaskLen.value ="";
				if(mapt_psid_offset.value == "0")
					mapt_psid_offset.value ="";
				if(mapt_psid_len.value == "0")
					mapt_psid_len.value ="";
				if(mapt_psid.value == "0x0")
					mapt_psid.value ="";
				break;
			default:
				break;
		}
	}
}

function mapeEnableFMR()
{
	with ( document.forms[0] ) 
	{
		if(mape_fmr_enabled.checked == true){
			document.ethwan.mape_fmr_enabled.value = "1";	
			if (document.getElementById('mape_fmr_list_show') != null)
				document.getElementById('mape_fmr_list_show').style.display="block";
		}	
		else{
			document.ethwan.mape_fmr_enabled.value = "0";
			if (document.getElementById('mape_fmr_list_show') != null)
				document.getElementById('mape_fmr_list_show').style.display="none";
		}	
	}	
}

function mapeFmrListShow(obj){
	var tmplst = "";
	
	if (document.ethwan.lkname.value == "new" )
	{		
		alert('This is a new link, please add the link first!');
		return false;
	}
	
	tmplst = curlink.name;
	document.ethwan.lst.value = tmplst;

	obj.isclick = 1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
	
	return true;
}

//MAP-E End
function protocolChange(UseDefaultSetting)
{
	ipver = document.ethwan.IpProtocolType.value;
	connectionMode = document.ethwan.adslConnectionMode.value;

	ipSettingsDisable();
	ipv6SettingsDisable();
	v6inv4Disable();
	
	if (ipver == 1)//ipv4 only
	{
		ipSettingsEnable();
   	}
   	else if (ipver == 2) //IPV6 only
   	{
		ipv6SettingsEnable(UseDefaultSetting);
	}
	else 
	{
		ipSettingsEnable();
		ipv6SettingsEnable(UseDefaultSetting);
	}
}
/* Mason Yu:20110307 END */

function on_linkchange(itlk)
{
	var pmchkpt = document.getElementById("tbl_pmap");
	temp_user_mvid_value = 0;	
	
	with ( document.forms[0] )
	{
		if (<% checkWrite("wan_logic_port"); %>)
			var wan_port_mask = <% checkWrite("wan_port_mask"); %>
		if(itlk == null)
		{
			mtu.value=1500;
			//select
			adslConnectionMode.value = pppConnectType.value = 0;
			//wan_port
			if (<% checkWrite("wan_logic_port"); %>)
			{
				for(var i=0; i< sw_lan_port_num; i++)
				{
					if(wan_port_mask & (0x01 << i))
						logic_port.options[i].disabled = true;
					else
					{
						logic_port.options[i].disabled = false;
						logic_port.value= i;
					}
				}
				//logic_port.value= <% checkWrite("sw_lan_port_num"); %> -1;
			}
			if(<% checkWrite("phy_rate"); %>)
			{
				phy_rate.value = 0;
			}

			if(typeof brmode != "undefined") // "undefined" refer to an object is not defined and defined but without a value. Be careful!
				brmode.value = 0;

			if(<% checkWrite("ConfigIPv6"); %>)
			IpProtocolType.value = 1;
			if (<% checkWrite("IPv6Show"); %>)
			AddrMode.value = 1;
			if (<% checkWrite("6rdShow"); %>) {
				SixrdMode[0].checked = true;
				SixrdBRv4IP.value = "";
				SixrdIPv4MaskLen.value = "";
				SixrdPrefix.value = "";
				SixrdPrefixLen.value = "";
			}
			// ctype
			ctype.value = 2;

			if (<% checkWrite("ConfigIPv6"); %>)
				v6TunnelType.value = 0;
			if (<% checkWrite("6in4Tunnelshow"); %>) {
				dnsV6Mode6in4[1].checked =true;
			}

			//radio
			ipMode[0].checked = droute[0].checked = dnsMode[1].checked = dnsV6Mode[1].checked =true;
			chEnable[0].checked = true;
			dslite_aftr_mode[0].checked = true;
			if(typeof duid_type !== 'undefined')
				duid_type[1].checked = true;

			//checkbox
			naptEnabled.checked = vlan.checked = igmpEnabled.checked = mldEnabled.checked = false;
			if(typeof enable_opt_60 !== 'undefined')
				enable_opt_60.checked = enable_opt_61.checked = enable_opt_125.checked = false;

			//input number
			vprio.value = vid.value = "0";
			vid.value = "";
			//input muticast vid
			multicast_vid.value = "";

			//macclone
			macclone.value = "0";
			document.getElementById("tb_macclone").style.display = "none";
			maccloneaddr.value = "";
			
			//input ip
			ip.value = remoteIp.value = "0.0.0.0";
			netmask.value = "255.255.255.0";


			//input text
			pppUserName.value = pppPassword.value = acName.value = serviceName.value = dns1.value = dns2.value = "";
			auth.value = 0;

			//checkbox
			Ipv6Addr.value = "";
			Ipv6PrefixLen.value = "";
			Ipv6Gateway.value = "";
			dnsV6Mode[0].checked = true;
			disableDNSv6Input();

			iana.checked = true;
			iapd.checked = false;

			if(typeof document.ethwan.br != "undefined") // "undefined" refer to an object is not defined and defined but without a value. Be careful!
				document.ethwan.br.checked = false;

			//port mapping
			if (pmchkpt)
			{
				if (<% checkWrite("wan_logic_port"); %>)
				{
					for (var i = 0; i < (sw_lan_port_num+10); i++) {
						/* chkpt do not always have 14 elements */
						if (!chkpt[i])
							break;
						chkpt[i].checked = false;
					}
				}
				else
				{
					for (var i = 0; i < 14; i++) {
						/* chkpt do not always have 14 elements */
						if (!chkpt[i])
							break;
						chkpt[i].checked = false;
					}
				}
			}	
		}
		else
		{
			//sji_onchanged(document, itlk);
			mtu.value=itlk.mtu;
			mtu_pppoe.value=itlk.mtu_pppoe;
			//wan_port
			if (<% checkWrite("wan_logic_port"); %>)
			{
				for(var i=0; i< sw_lan_port_num; i++)
				{
					if((wan_port_mask & (0x01 << i))&&(i != itlk.logic_port))
						logic_port.options[i].disabled = true;
					else
						logic_port.options[i].disabled = false;
				}
				logic_port.value=itlk.logic_port;
			}
			//wan limit rate
			if (<% checkWrite("phy_rate"); %>)
			{
				phy_rate.value=itlk.phy_rate;
			}
			
			//select
			adslConnectionMode.value = itlk.cmode;
			if (<% checkWrite("IPv6Show"); %>)
			{
				AddrMode.value = itlk.AddrMode;
				IpProtocolType.value = itlk.IpProtocol;
				v6TunnelType.value = itlk.v6TunnelType;
			}
			//ctype
			if (itlk.cmode == 0)
				ctypeForBridge.value = itlk.applicationtype;
			else
				ctype.value = itlk.applicationtype;

			//brmode
			if(document.ethwan.br)
			{
				document.ethwan.br.checked = false;
				if(itlk.brmode == 2 || itlk.brmode == 0)
				{
					// Disable Bridge
					brmode.value = 0;
					brmode.disabled = true;
				}
				else
				{
					// Enable Bridge
					if(itlk.cmode != 0)
						document.ethwan.br.checked = true;

					brmode.value = itlk.brmode;
					brmode.disabled = false;
				}
			}
			macclone.value = itlk.macclone_enable;
			if(macclone.value == 0){
				document.getElementById("tb_macclone").style.display = "none";
			}
			else
			{
				document.getElementById("tb_macclone").style.display = "";
				maccloneaddr.value = itlk.macclone_addr;
			}
			//checkbox
			if (itlk.napt == 1)
				naptEnabled.checked = true;
			else
				naptEnabled.checked = false;
			if (itlk.enableIGMP == 1)
				igmpEnabled.checked = true;
			else
				igmpEnabled.checked = false;
			if (itlk.enableMLD == 1)
				mldEnabled.checked = true;
			else
				mldEnabled.checked = false;
			mtu.value = itlk.mtu;
			mtu_pppoe.value = itlk.mtu_pppoe;
			if (itlk.vlan == 1)
			{
				vlan.checked = true;
				vid.value = itlk.vid;
				vprio.value = itlk.vprio;
			}
			else
			{
				vlan.checked = false;
			}
			
			/* WAN service is TR069, Voice, TR069 and Voice*/
			if(<% checkWrite("mcastVlan"); %>) {
				if (itlk.applicationtype == 1 || itlk.applicationtype == 8 || itlk.applicationtype == 9)
					document.getElementById("multicast_vid_tr").style.display = "none";
				else
				{
					document.getElementById("multicast_vid_tr").style.display = "";
					if (itlk.mVid == 0) /* Because default mVid is 0, it means set "" for user. */
						multicast_vid.value = "";
					else
						multicast_vid.value = itlk.mVid;
				}
			}

			//radio
			if (itlk.dgw == 1)
				droute[1].checked = true;
			else
				droute[0].checked = true;
			if (itlk.enable == 1)
				chEnable[0].checked = true;
			else
				chEnable[1].checked = true;

			//radio
			// if(itlk.cmode != 0)//not bridge
			if(itlk.cmode != -1)//not bridge
			{
				if(<% checkWrite("ConfigIPv6"); %>)
				{
				IpProtocolType.value = itlk.IpProtocol;
				v6TunnelType.value = itlk.v6TunnelType;
				s4over6Type.value = itlk.s4over6Type;
				if (IpProtocolType.value != 1 || v6TunnelType.value == 1 || v6TunnelType.value == 2 || v6TunnelType.value == 3)
				{
					if (itlk.AddrMode & 2)
					{
						Ipv6Addr.value = itlk.Ipv6Addr;
						Ipv6PrefixLen.value = itlk.Ipv6AddrPrefixLen;
						Ipv6Gateway.value = itlk.RemoteIpv6Addr;
					}
					else
					{
						Ipv6Addr.value = "";
						Ipv6PrefixLen.value = "";
						Ipv6Gateway.value = "";
					}

					if (itlk.dnsv6Mode == 1) 
					{
						dnsV6Mode[0].checked = true;
						disableDNSv6Input();
					}
					else
					{
						dnsV6Mode[1].checked = true;
						dnsModeV6Clicked();
					}
					Ipv6Dns1.value = itlk.Ipv6Dns1;
					Ipv6Dns2.value = itlk.Ipv6Dns2;

					if (<% checkWrite("6in4Tunnelshow"); %>) {
						if (v6TunnelType.value == 1)  // 6rd
						{
							Ipv6Dns16rd.value = itlk.Ipv6Dns1;
							Ipv6Dns26rd.value = itlk.Ipv6Dns2;
						}
						else {
							Ipv6Dns16rd.value = "";
							Ipv6Dns26rd.value = "";
						}

						if (v6TunnelType.value == 2)  // 6in4
						{
							Ipv6Addr6in4.value = itlk.Ipv6Addr;
							Ipv6PrefixLen6in4.value = itlk.Ipv6AddrPrefixLen;
							Ipv6Gateway6in4.value = itlk.RemoteIpv6Addr;
							v6TunnelRv4IP6in4.value = itlk.v6TunnelRv4IP;

							if (itlk.dnsv6Mode == 1)
							{
								dnsV6Mode6in4[0].checked = true;
								disableDNSv6Input6in4();
							}
							else
							{
								dnsV6Mode6in4[1].checked = true;
								dnsModeV6Clicked6in4();
							}
							Ipv6Dns16in4.value = itlk.Ipv6Dns1;
							Ipv6Dns26in4.value = itlk.Ipv6Dns2;
							}
						else
						{
							Ipv6Addr6in4.value = "";
							Ipv6PrefixLen6in4.value = "";
							Ipv6Gateway6in4.value = "";
							Ipv6Dns16in4.value = "";
							Ipv6Dns26in4.value = "";
							dnsV6Mode6in4[0].checked = true;
							dnsV6Mode6in4[1].checked = false;
						}

						if (v6TunnelType.value == 3)  // 6to4
						{
							v6TunnelRv4IP6to4.value = itlk.v6TunnelRv4IP;
							Ipv6Dns16to4.value = itlk.Ipv6Dns1;
							Ipv6Dns26to4.value = itlk.Ipv6Dns2;
						}
						else {
							v6TunnelRv4IP6to4.value = "";
							Ipv6Dns16to4.value = "";
							Ipv6Dns26to4.value = "";
						}
					}

//					if (itlk.Ipv6Dhcp)
//					{
						if (itlk.Ipv6DhcpRequest & 1)
							iana.checked = true;
						else
							iana.checked = false;
						if (itlk.Ipv6DhcpRequest & 2)
							iapd.checked = true;
						else
							iapd.checked = false;
//					}
					napt_v6.selectedIndex = itlk.napt_v6;
					ndp_proxy.checked = itlk.ndp_proxy;
					
					// DS-Lite
					if ((<% checkWrite("DSLiteShow"); %>) && (IpProtocolType.value == 2)) { // IPv6 only
						if (itlk.dslite_aftr_mode == 0)
							dslite_aftr_mode[0].checked = true;
						else
							dslite_aftr_mode[1].checked = true;
						dslite_aftr_hostname.value = itlk.dslite_aftr_hostname;
					}

					// MAP-E
					if ((<% checkWrite("MAPEShow"); %>) && (IpProtocolType.value == 2)) { // IPv6 only
						if (itlk.mape_enable == 1){
							mapeSettingChange();
							
							//nat mode
							if (mape_routing_mode_support){
								if (itlk.mape_forward_mode == 1)//Routing mode
									mape_forward_mode[1].checked = true;
								else //NAT mode
									mape_forward_mode[0].checked = true;
								mapeForwardModeChange();
							}	
							mape_mode.value = itlk.mape_mode;
							mapeModeChange();
							if (mape_mode.value == 1){//static
								//DMR
								mape_remote_v6Addr.value = itlk.mape_remote_v6Addr;
								
								//BMR	
								mape_local_v6Prefix.value = itlk.mape_local_v6Prefix;
								mape_local_v6PrefixLen.value = itlk.mape_local_v6PrefixLen;
								mape_local_v4Addr.value = itlk.mape_local_v4Addr;
								mape_local_v4MaskLen.value = itlk.mape_local_v4MaskLen;
								mape_psid_offset.value = itlk.mape_psid_offset;
								mape_psid_len.value = itlk.mape_psid_len;
								mape_psid.value = Dec_toHex(itlk.mape_psid);
								//FMR
								if (itlk.mape_fmr_enabled == 0)
									mape_fmr_enabled.checked = false;
								else
									mape_fmr_enabled.checked = true;
								mapeEnableFMR();
							}
							
						}
					}

					// MAP-T
					if ((<% checkWrite("MAPTShow"); %>) && (IpProtocolType.value == 2)) { // IPv6 only
						if(itlk.mapt_enable){
							mapt_mode.value = itlk.mapt_mode;
							mapt_local_v6Prefix.value = itlk.mapt_local_v6Prefix;
							mapt_local_v6PrefixLen.value = itlk.mapt_local_v6PrefixLen;
							mapt_remote_v6Prefix.value = itlk.mapt_remote_v6Prefix;
							mapt_remote_v6PrefixLen.value = itlk.mapt_remote_v6PrefixLen;
							mapt_local_v4Addr.value = itlk.mapt_local_v4Addr;
							mapt_local_v4MaskLen.value = itlk.mapt_local_v4MaskLen;
							mapt_psid_offset.value = itlk.mapt_psid_offset;
							mapt_psid_len.value = itlk.mapt_psid_len;
							mapt_psid.value = Dec_toHex(itlk.mapt_psid);
						}
					}

					// XLAT464
					if ((<% checkWrite("XLAT464Show"); %>) && (IpProtocolType.value == 2)) {
						if (itlk.xlat464_clat_mode == 0)
							xlat464_clat_mode[0].checked = true;
						else
							xlat464_clat_mode[1].checked = true;
						if(xlat464_clat_mode.value == 1)//static
							xlat464_clat_prefix6.value = itlk.xlat464_clat_prefix6;
						xlat464_plat_prefix6.value = itlk.xlat464_plat_prefix6;
					}

					// lw4o6
					if ((<% checkWrite("LW4O6Show"); %>) && (IpProtocolType.value == 2)) {
						if (itlk.lw4o6_mode == 0)
							lw4o6_mode[0].checked = true;
						else
							lw4o6_mode[1].checked = true;
						if(lw4o6_mode.value == 1){//static
							lw4o6_aftr_addr.value = itlk.lw4o6_aftr_addr;
							lw4o6_local_v6Prefix.value = itlk.lw4o6_local_v6Prefix;
							lw4o6_local_v6PrefixLen.value = itlk.lw4o6_local_v6PrefixLen;
							lw4o6_v4addr.value = itlk.lw4o6_v4addr;
							lw4o6_psid_len.value = itlk.lw4o6_psid_len;
							lw4o6_psid.value = Dec_toHex(itlk.lw4o6_psid);
						}
					}

					// 6rd
					if (itlk.AddrMode & 8)
					{
						SixrdBRv4IP.value = itlk.SixrdBRv4IP;
						SixrdIPv4MaskLen.value = itlk.SixrdIPv4MaskLen;
						SixrdPrefix.value = itlk.SixrdPrefix;
						SixrdPrefixLen.value = itlk.SixrdPrefixLen;
						ip.value = itlk.ipAddr;
						remoteIp.value = itlk.remoteIpAddr;
						netmask.value = itlk.netMask;
						if (itlk.SixrdMode == 1) {
							SixrdMode[0].checked = true;
							SixrdBRv4IP.value = "";
							SixrdIPv4MaskLen.value = "";
							SixrdPrefix.value = "";
							SixrdPrefixLen.value = "";
						}
						else
							SixrdMode[1].checked = true;
					}

				}else{
					Ipv6Addr.value = "";
					Ipv6PrefixLen.value = "";
					Ipv6Gateway.value = "";
					dnsV6Mode[0].checked = true;
					disableDNSv6Input();
					iana.checked = true;
					iapd.checked = false;
				}
				}

				if (itlk.cmode == 0 || itlk.cmode == 1 || itlk.cmode == 8 || itlk.cmode ==9 || itlk.cmode ==10 )//IPoE or 6RD or 6in4
				{
					if (itlk.ipDhcp == 1)
					{
						ipMode[0].checked = false;
						ipMode[1].checked = true;
						ip.value = "";
						remoteIp.value = "";
						netmask.value = "";
					}
					else
					{
						ipMode[0].checked = true;
						ipMode[1].checked = false;
						ip.value = itlk.ipAddr;
						remoteIp.value = itlk.remoteIpAddr;
						netmask.value = itlk.netMask;
					}
					if (itlk.dnsMode == 1)
							dnsMode[0].checked = true;
						else
							dnsMode[1].checked = true;
					dns1.value = itlk.v4dns1;
					dns2.value = itlk.v4dns2;
				}
				else if (itlk.cmode == 2)
				{
					pppUserName.value = decode64(itlk.pppUsername);
					pppUserName.value = unescapeHTML(pppUserName.value);
					pppPassword.value = decode64(itlk.pppPassword);
					pppConnectType.value = itlk.pppCtype;
					pppIdleTime.value = itlk.pppIdleTime;
					auth.value = itlk.pppAuth;
					acName.value = itlk.pppACName;
					serviceName.value = itlk.pppServiceName;
				}
				if(<% checkWrite("ConfigIPv6"); %>)
				protocolChange(0);
			}

			//DHCP options
			if(typeof enable_opt_60 !== 'undefined')
			{
				//assume all other elements are existed if enable_opt_60 is existed

				if(itlk.enable_opt_60)
					enable_opt_60.checked = true;

				opt60_val.value = itlk.opt60_val;

				if(itlk.enable_opt_61)
					enable_opt_61.checked = true;

				iaid.value = itlk.iaid;

				if(itlk.duid_type == 0)
					duid_type[0].checked = true;
				else
					duid_type[itlk.duid_type - 1].checked = true;
				duid_ent_num.value = itlk.duid_ent_num;
				duid_id.value = itlk.duid_id;

				if(itlk.enable_opt_125)
					enable_opt_125.checked = true;

				manufacturer.value = itlk.manufacturer;
				product_class.value = itlk.product_class;
				model_name.value = itlk.model_name;
				serial_num.value = itlk.serial_num;
			}
			//port mapping
			if (pmchkpt)
			{
				if (<% checkWrite("wan_logic_port"); %>)
				{
					for (var i = 0; i < (sw_lan_port_num+10); i++) {
						/* chkpt do not always have 14 elements */
						if (!chkpt[i])
							break;
						chkpt[i].checked = (itlk.itfGroup & (0x1 << i));
					}
				}
				else
				{
					for (var i = 0; i < 14; i++) {
						/* chkpt do not always have 14 elements */
						if (!chkpt[i])
							break;
						chkpt[i].checked = (itlk.itfGroup & (0x1 << i));
					}
				}
				
			}
			
				
		}
	}
	if(<% checkWrite("ConfigIPv6"); %>)
	ipver = document.ethwan.IpProtocolType.value;

	vlanClicked();
	autoDGWclicked();
	if (<% checkWrite("wan_logic_port"); %>)
		wanLogicPortSelection();
	adslConnectionModeSelection(true);
	connectionTypeChangeDisplay();
	v6inv4TunnelSel();
}

function on_ctrlupdate()
{
	if(<% checkWrite("mcastVlan"); %>)
			document.getElementById("multicast_vid_tr").style.display = "";
	with ( document.forms[0] )
	{
		if(lkname.value == "new")
		{
			curlink = null;
			on_linkchange(curlink);
		}
		else
		{
			curlink = links[lkname.value];
			on_linkchange(curlink);
		}
	}
}

function wan_service_change()
{
	temp_user_mvid_value = document.getElementById("multicast_vid").value;

	if(<% checkWrite("ConfigIPv6"); %>){
		dynamic_create_ipv6_addrmode_options();
		wanAddrModeChange(1);
	}

	/* WAN service is TR069, Voice, TR069 and Voice*/
	if (isLocalWan()) //TR069, VOIP and TR069_VOIP
	{
		if(<% checkWrite("mcastVlan"); %>)
			document.getElementById("multicast_vid_tr").style.display = "none";
	}
	else
	{
		if(<% checkWrite("mcastVlan"); %>)
			document.getElementById("multicast_vid_tr").style.display = "";
		if (curlink.mVid == 0 || temp_user_mvid_value != 0)
		{
			/* If user set mVid , remember user's setting instead of mib mVid value. */
			document.getElementById("multicast_vid").value = temp_user_mvid_value;
		}
		else
			document.getElementById("multicast_vid").value = curlink.mVid;
	}
}
function wan_macclone_change()
{
	var macclone_value = document.ethwan.macclone.value;
	if (macclone_value == 0)
	{	
		document.getElementById("tb_macclone").style.display = "none";
	}
	else if(macclone_value == 1)
	{
		document.getElementById("tb_macclone").style.display = "";
	} 
	else if(macclone_value == 2)
	{
		document.getElementById("tb_macclone").style.display = "";
		document.ethwan.maccloneaddr.value = "<% checkWrite("pc_mac"); %>";
	}

}

function on_init()
{
	sji_docinit(document, cgi);

	with ( document.forms[0] )
	{
		for(var k in links)
		{
			var lk = links[k];
			lkname.options.add(new Option(lk.name, k));
		}
		lkname.options.add(new Option("new link", "new"));

		if(links.length > 0) lkname.selectedIndex = 0;
		on_ctrlupdate();
	}
}

/*
function SubmitWANMode() // Magician: ADSL/Ethernet WAN mode switch
{
	var wmmap = 0;
	var config_num = 4;

	with (document.forms[0])
	{
		for(var i = 0; i < config_num; i ++)
			if(wmchkbox[i].checked == true)
				wmmap |= (0x1 << i);

		if(wmmap == 0 || wmmap == wanmode)
			return false;

		wan_mode.value = wmmap;
	}

	return confirm("It needs rebooting to change WAN mode.");
}*/
</script>

</head>
<BODY onLoad="on_init();RegisterClickEvent();">
<div class="intro_main ">
	<p class="intro_title"><% getWanIfDisplay(); %> <% multilang(LANG_WAN); %></p>
	<p class="intro_content"> <% multilang(LANG_PAGE_DESC_CONFIGURE_PARAMETERS); %><% getWanIfDisplay(); %><% multilang(LANG_WAN); %></p>
</div>
<form action=/boaform/admin/formWanEth method=POST name="ethwan">
<input type=hidden name=v6TunnelRv4IP value="">
<!--<table border="0" cellspacing="4" width="800" <% WANConditions(); %>>
	<tr>
		<td>
			<b><% multilang(LANG_WAN_MODE); %>:</b>
			<span <% checkWrite("wan_mode_atm"); %>><input type="checkbox" value=1 name="wmchkbox">ATM</span>
			<span <% checkWrite("wan_mode_ethernet"); %>><input type="checkbox" value=2 name="wmchkbox">Ethernet</span>
			<span <% checkWrite("wan_mode_ptm"); %>><input type="checkbox" value=4 name="wmchkbox">PTM</span>
			<span <% checkWrite("wan_mode_bonding"); %>><input type="checkbox" value=8 name="wmchkbox">Bonding</span>&nbsp;&nbsp;&nbsp;&nbsp;
			<input type="hidden" name="wan_mode" value=0>
			<input type="submit" value="Submit" name="submitwan" onClick="return SubmitWANMode()">
		</td>
	</tr>
	<tr><td><hr size=1 noshade align=top></td></tr>
</table>-->

<div class="data_common data_common_notitle">
<table>
	<tr>
		<th colspan=2><select name="lkname" onChange="on_ctrlupdate()" size="1"></th>
	</tr>
	<tr>
		<th width=30%><% multilang(LANG_ENABLE_VLAN); %>: </th>
		<td width=70%><input type="checkbox" name="vlan" size="2" maxlength="2" value="ON" onClick=vlanClicked()></td>
	</tr>
	<tr>
		<th><% multilang(LANG_VLAN); %> ID:</th>
		<td><input type="text" name="vid" size="10" maxlength="15"></td>
	</tr>
	<tr>
		<th><% multilang(LANG_802_1P_MARK); %>  </th>
		<td><select style="WIDTH: 60px" name="vprio">
			<option value="0" >   </option>
			<option value="1" > 0 </option>
			<option value="2" > 1 </option>
			<option value="3" > 2 </option>
			<option value="4" > 3 </option>
			<option value="5" > 4 </option>
			<option value="6" > 5 </option>
			<option value="7" > 6 </option>
			<option value="8" > 7 </option>
			</select>
		</td>
	</tr>
	<tr id="multicast_vid_tr" style="display:none">
		<th><% multilang(LANG_MCAST_VLAN); %> ID: [1-4095]</th>
		<td><input type="text" id="multicast_vid" name="multicast_vid" size="10" maxlength="15"></td>
	</tr>
	<tr>
		<% ShowChannelMode("ethcmode"); %>
	</tr>
	<tr>
		<% ShowWanPortSetting(); %>
	</tr>

	<% ShowBridgeMode(); %>
	<tr>
		<% ShowNAPTSetting(); %>
	</tr>
	<tr>
		<th><% multilang(LANG_ADMIN_STATUS); %>:</th>
		<td><input type=radio value=1 name="chEnable"><% multilang(LANG_ENABLE); %>
			<input type=radio value=0 name="chEnable" checked><% multilang(LANG_DISABLE); %>
		</td>
	</tr>
	<% ShowConnectionType(); %>
	<% ShowConnectionTypeForBridge(); %>

	<tr id=tbmtu>
		<th>MTU: [1280-2000] </th>
		<td>
		<input type="text" name="mtu" size="10" maxlength="15">
		</td>
	</tr>
	<tr id=tbmtu_pppoe>
		<th>MTU: [1280-1492] </th>
		<td>
		<input type="text" name="mtu_pppoe" size="10" maxlength="15">
		</td>
	</tr>
	<% ShowMacClone(); %>
	<tr id=tb_macclone style="display:none">
    	<th>Mac <% multilang(LANG_CLONE_ADDRESS); %>:  </th>
    	<td>
    	<input type="text" name="maccloneaddr" size="12" maxlength="15" >
    	</td>
  	</tr>
	<tr ID=dgwshow style="display:none">
		<th><% multilang(LANG_DEFAULT_ROUTE); %>:</th>
		<td>
			<input type=radio value=0 name="droute"><% multilang(LANG_DISABLE); %>
			<input type=radio value=1 name="droute" checked><% multilang(LANG_ENABLE); %>
		</td>
	</tr>
	<tr ID=IGMPProxy_show style="display:none">
		<% ShowIGMPSetting(); %>
	</tr>
	<tr>
		<% ShowMLDSetting(); %>
	</tr>
	<% ShowIpProtocolType(); %>
</table>
</div>

<% ShowPPPIPSettings(); %>
<% ShowDefaultGateway("p2p"); %>
<% Showv6inv4TunnelSetting(); %>

<% Show6rdSetting(); %>
<% Show6in4Setting(); %>
<% Show6to4Setting(); %>
<% ShowIPV6Settings(); %>
<% ShowPortMapping(); %>

<div class="btn_ctl">
<input type="hidden" value="/admin/multi_wan_generic.asp" name="submit-url">
<input type="hidden" id="lst" name="lst" value="">
<input type="hidden" name="encodePppUserName" value="">
<input type="hidden" name="encodePppPassword" value="">
<input type="hidden" name="qosEnabled" value="">
<input class="link_bg" type="submit" value="<% multilang(LANG_APPLY_CHANGES); %>" name="apply" onClick="return applyCheck(this)">&nbsp; &nbsp; &nbsp; &nbsp;
<input class="link_bg" type="submit" value="<% multilang(LANG_DELETE); %>" name="delete" onClick="return deleteCheck(this)">
<input type="hidden" name="itfGroup" value=0>
<input type="hidden" name="postSecurityFlag" value="">
</div>
<script>
	<% DisplayDGW(); %>

/*
	var wanmode = <% getInfo("wan_mode"); %>;

	if((wanmode & 1) == 1)
		document.ethwan.wmchkbox[0].checked = true;

	if((wanmode & 2) == 2)
		document.ethwan.wmchkbox[1].checked = true;

	if((wanmode & 4) == 4)
		document.ethwan.wmchkbox[2].checked = true;

	if((wanmode & 8) == 8)
		document.ethwan.wmchkbox[3].checked = true;
*/

</script>
</form>
</blockquote>
</body>
</html>
