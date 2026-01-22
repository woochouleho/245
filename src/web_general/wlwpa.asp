<% SendWebHeadStr();%>
<title><% multilang(LANG_WLAN_SECURITY_SETTINGS); %></title>
<style>
.on {display:on}
.off {display:none}	

</style>
<script type="text/javascript" src="base64_code.js"></script>
<script>
var defPskLen, defPskFormat;
var wps20, ssid_num;
var wpa3_disable_wps = 0;
var wps20_use_version=<% getInfo("wpsUseVersion"); %>;
var wlan_idx=<% checkWrite("wlan_idx"); %>;
var oldMethod;
var wlanMode;
var _wlan_mode=new Array();
var _encrypt=new Array();
var _enable1X=new Array();
var _wpaAuth=new Array();
var _wpaPSKFormat=new Array();
var _wpaPSK=new Array();
var _wpaGroupRekeyTime=new Array();
var _rsPort=new Array();
var _rsIpAddr=new Array();
var _rsPassword=new Array();
var _rs2Port=new Array();
var _rs2IpAddr=new Array();
var _rs2Password=new Array();
var _uCipher=new Array();
var _wpa2uCipher=new Array();
var _wepAuth=new Array();
var _wepLen=new Array();
var _wepKeyFormat=new Array();
var _wepKeyValue=new Array();
var _wepKey128Value=new Array();
var _wlan_isNmode=new Array();
var _ssid_band=new Array();
var new_wifi_sec=<% checkWrite("new_wifi_security"); %>;
var support_11w=<% checkWrite("11w_support"); %>;
var _dotIEEE80211W=new Array();
var _sha256=new Array();
var support_wpa3_h2e=<% checkWrite("wpa3_h2e_support"); %>;
var _wpa3_sae_pwe=new Array();
var wlan6gSupport = 0;
var is_wlan_qtn = <% checkWrite("is_wlan_qtn"); %>;
var wepkeyform;
var is_radius_2set = <% checkWrite("is_wlan_radius_2set"); %>;
var support_1x=<% checkWrite("1x_support"); %>;
var enable_wpa2_and_wpa3_1x_only=<% checkWrite("enable_wpa2_and_wpa3_1x_only"); %>;
var support_wpa3_1x=<% checkWrite("wpa3_1x_support"); %>;

function show_8021x_settings()
{
	var security = get_by_id("security_method");
	var enable_1x = get_by_id("use1x");
	var form1 = document.formEncrypt ;
	var dF=document.forms[0];
	if (enable_1x.checked) {		
		if (security.value == 1)	/* WIFI_SEC_WEP */
			get_by_id("show_1x_wep").style.display = "";
		else 
			get_by_id("show_1x_wep").style.display = "none";
		get_by_id("setting_wep").style.display = "none";		
		get_by_id("show_8021x_eap").style.display = "";
		if(is_radius_2set == 1)
			get_by_id("show2_8021x_eap").style.display = "";
		dF.auth_type[2].checked = true;
		dF.auth_type[0].disabled = true;
		dF.auth_type[1].disabled = true;
		dF.auth_type[2].disabled = true;
	}
	else {	
		if (security.value == 1)	/* WIFI_SEC_WEP */
			get_by_id("setting_wep").style.display = "";	
		else	
			get_by_id("setting_wep").style.display = "none";	

		get_by_id("show_1x_wep").style.display = "none";			
		get_by_id("show_8021x_eap").style.display = "none";
		if(is_radius_2set == 1)
			get_by_id("show2_8021x_eap").style.display = "none";

		if (security.value == 2 || security.value == 4 || security.value == 6){		/* WIFI_SEC_WPA/WIFI_SEC_WPA2/WIFI_SEC_WPA2_MIXED */
			if(dF.wpaAuth[1].checked==true)
				get_by_id("show_8021x_eap").style.display = "none";
			else
				get_by_id("show_8021x_eap").style.display = "";

			if(is_radius_2set == 1){
				if(dF.wpaAuth[1].checked==true)
					get_by_id("show2_8021x_eap").style.display = "none";
				else
					get_by_id("show2_8021x_eap").style.display = "";
			}
		}	
		//get_by_id("show_8021x_eap").style.display = "none";
		//dF.auth_type[2].checked = true;
		dF.auth_type[0].disabled = false;
		dF.auth_type[1].disabled = false;
		dF.auth_type[2].disabled = false;
	}		
}
function show_wpa3_h2e_settings()
{
	var dF=document.forms[0];
	if(support_wpa3_h2e){
		if (dF.security_method.value == 16 || dF.security_method.value == 20)
			get_by_id("show_wpa3_sae_pwe").style.display = "";

		if(wlan6gSupport){
			if(dF.security_method.value == 16){
				disableRadioGroup(dF.wpa3_sae_pwe);
				dF.wpa3_sae_pwe[1].checked = true;
			}
		}
		else{
			if(dF.security_method.value == 20){
				disableRadioGroup(dF.wpa3_sae_pwe);
				dF.wpa3_sae_pwe[0].checked = true;
			}
			else if(dF.security_method.value == 16){
				disableRadioGroup(dF.wpa3_sae_pwe);
				dF.wpa3_sae_pwe[1].checked = true;
			}
		}
	}
}

function show_wpa_settings()
{
	var dF=document.forms[0];
	var allow_tkip=0;

	get_by_id("show_wpa_gk_rekey").style.display = "";
	get_by_id("show_wpa_psk1").style.display = "none";
	get_by_id("show_wpa_psk2").style.display = "none";	
	get_by_id("show_8021x_eap").style.display = "none";
	if(is_radius_2set == 1)
		get_by_id("show2_8021x_eap").style.display = "none";

	if(support_wpa3_h2e){
		get_by_id("show_wpa3_sae_pwe").style.display = "none";
		enableRadioGroup(dF.wpa3_sae_pwe);
	}
	
	if (dF.wpaAuth[1].checked)
	{
		get_by_id("show_wpa_psk1").style.display = "";
		get_by_id("show_wpa_psk2").style.display = "";		
		show_wpa3_h2e_settings();
	}
	else{
		if (wlanMode != 1)
		get_by_id("show_8021x_eap").style.display = "";

		if(is_radius_2set == 1){
			if (wlanMode != 1)
				get_by_id("show2_8021x_eap").style.display = "";
		}
	}	
}

function show_wapi_settings()
{
        var dF=document.forms[0];
        
        get_by_id("show_wapi_psk1").style.display = "none";
        get_by_id("show_wapi_psk2").style.display = "none";
        get_by_id("show_8021x_wapi").style.display = "none";
        
        if (dF.wapiAuth[1].checked){
                get_by_id("show_wapi_psk1").style.display = "";
                get_by_id("show_wapi_psk2").style.display = "";
        }
        else{
                if (wlanMode != 1)
                {
                	get_by_id("show_8021x_wapi").style.display = "";
			if(''=='true')
			{
				get_by_id("show_8021x_wapi_local_as").style.display = "";
			}
			else
			{
				get_by_id("show_8021x_wapi_local_as").style.display = "none";
				dF.uselocalAS.checked=false;
			}
                }
		if (dF.wapiASIP.value == "192.168.1.1")
		{
			dF.uselocalAS.checked=true;
		}
        }
}

function show_wapi_ASip()
{
	var dF=document.forms[0];
	if (dF.uselocalAS.checked)
	{
		dF.wapiASIP.value = "192.168.1.1";
        }
	else
	{
		dF.wapiASIP.value = "";
	}
}

function show_sha256_settings()
{
	if(document.formEncrypt.dotIEEE80211W[1].checked == true)
		get_by_id("show_sha256").style.display = "";
	else
		get_by_id("show_sha256").style.display = "none";
}
	
function show_authentication(on_change)
{	
	var ssid_idx=document.formEncrypt.wpaSSID.value;
	var security = get_by_id("security_method");
	var enable_1x = get_by_id("use1x");	
	var form1 = document.formEncrypt ;

	if(_ssid_band[ssid_idx]>3) {//11a, 11b,11g, 11bg
		if (security.value == 1){	/* WIFI_SEC_WEP */
			alert("<% multilang(LANG_ERROR_BAND_NOT_SPPORT_WEP); %>");
			window.location.reload(true);
			return;
		}
	}
	
	if (wlanMode==1 && security.value == 6) {	/* client and WIFI_SEC_WPA2_MIXED */
		alert("<% multilang(LANG_NOT_ALLOWED_FOR_THE_CLIENT_MODE); %>");
		security.value = oldMethod;
		return false;
	}
	oldMethod = security.value;
	get_by_id("show_wep_auth").style.display = "none";	
	get_by_id("setting_wep").style.display = "none";
	get_by_id("setting_wpa").style.display = "none";
	get_by_id("setting_wapi").style.display = "none";
	get_by_id("show_wpa_cipher").style.display = "none";
	get_by_id("show_wpa2_cipher").style.display = "none";
	get_by_id("show_wpa3_cipher").style.display = "none";
	get_by_id("show_wpa_gk_rekey").style.display = "none";
	get_by_id("enable_8021x").style.display = "none";
	get_by_id("show_8021x_eap").style.display = "none";
	if(is_radius_2set == 1)
		get_by_id("show2_8021x_eap").style.display = "none";
	get_by_id("show_8021x_wapi").style.display = "none";
	get_by_id("show_1x_wep").style.display = "none";
        get_by_id("show_wapi_psk1").style.display = "none";
        get_by_id("show_wapi_psk2").style.display = "none";
        get_by_id("show_8021x_wapi").style.display = "none";
	get_by_id("show_wpaAuth").style.display = "none";
	if(support_11w){
		get_by_id("show_dotIEEE80211W").style.display = "none";
		get_by_id("show_sha256").style.display = "none";
		enableRadioGroup(form1.dotIEEE80211W);
		enableRadioGroup(form1.sha256);
		if(on_change){
			form1.sha256[0].checked = true;
			form1.dotIEEE80211W[1].checked = true;
		}
	}
	
	if (security.value == 1){	/* WIFI_SEC_WEP */
		get_by_id("show_wep_auth").style.display = "";		
		if (wlanMode == 1) 
			get_by_id("setting_wep").style.display = "";		
		else {
			if(support_1x == 1 && enable_wpa2_and_wpa3_1x_only == 0)
				get_by_id("enable_8021x").style.display = "";
			else
				enable_1x.checked = false;
			if(enable_1x.checked){
				get_by_id("show_8021x_eap").style.display = "";
				if(is_radius_2set == 1)
					get_by_id("show2_8021x_eap").style.display = "";
				get_by_id("show_1x_wep").style.display = "";
				get_by_id("setting_wep").style.display = "none";
				form1.auth_type[2].checked = true;
				form1.auth_type[0].disabled = true;
				form1.auth_type[1].disabled = true;
				form1.auth_type[2].disabled = true;
			}else{		
				get_by_id("setting_wep").style.display = "";
			}
		}
	
	}else if (security.value == 2 || security.value == 4 || security.value == 6){	/* WIFI_SEC_WPA/WIFI_SEC_WPA2/WIFI_SEC_WPA2_MIXED */
		form1.ciphersuite_t.disabled = false;
		form1.ciphersuite_a.disabled = false;
		form1.wpa2ciphersuite_t.disabled = false;
		form1.wpa2ciphersuite_a.disabled = false;
		get_by_id("setting_wpa").style.display = "";
		if (security.value == 2) {	/* WIFI_SEC_WPA */
			get_by_id("show_wpa_cipher").style.display = "";
			form1.wpa2ciphersuite_t.disabled = true;
			form1.wpa2ciphersuite_a.disabled = true;
			if ( form1.isNmode.value == 1 ) {
				//alert("Select wpa and is Nmode");
				form1.ciphersuite_t.disabled = true;
				form1.ciphersuite_t.checked = false;
				form1.wpa2ciphersuite_t.disabled = true;
				form1.wpa2ciphersuite_t.checked = false;
			}
		}
		if(security.value == 4) {	/* WIFI_SEC_WPA2 */
			get_by_id("show_wpa2_cipher").style.display = "";
			form1.ciphersuite_t.disabled = true;
			form1.ciphersuite_a.disabled = true;
			if(support_11w){
				get_by_id("show_dotIEEE80211W").style.display = "";
				if(form1.dotIEEE80211W[1].checked == true)
					get_by_id("show_sha256").style.display = "";
			}
			if(new_wifi_sec){
					form1.wpa2ciphersuite_t.disabled = true;
					form1.wpa2ciphersuite_t.checked = false;
					form1.wpa2ciphersuite_a.disabled = true;
					form1.wpa2ciphersuite_a.checked = true;
			}
			else{
				if ( form1.isNmode.value == 1 ) {
					//alert("Select wpa2 and is Nmode");
					form1.ciphersuite_t.disabled = true;
					form1.ciphersuite_t.checked = false;
					form1.wpa2ciphersuite_t.disabled = true;
					form1.wpa2ciphersuite_t.checked = false;
				}
			}
		}
		if(security.value == 6){	/* WIFI_SEC_WPA2_MIXED */
			get_by_id("show_wpa_cipher").style.display = "";
			get_by_id("show_wpa2_cipher").style.display = "";
			if(new_wifi_sec){
				form1.ciphersuite_t.disabled = true;
				form1.ciphersuite_t.checked = true;
				form1.ciphersuite_a.disabled = true;
				form1.ciphersuite_a.checked = true;
				form1.wpa2ciphersuite_t.disabled = true;
				form1.wpa2ciphersuite_t.checked = true;
				form1.wpa2ciphersuite_a.disabled = true;
				form1.wpa2ciphersuite_a.checked = true;
			}
			else{
				form1.ciphersuite_t.disabled = false;
				form1.ciphersuite_a.disabled = false;
				form1.wpa2ciphersuite_t.disabled = false;
				form1.wpa2ciphersuite_a.disabled = false;
			}
		}

		/*check 1x support for WPA/WPA2/WPA2 mixed */
		if(support_1x == 1){
			if(enable_wpa2_and_wpa3_1x_only == 0)
				get_by_id("show_wpaAuth").style.display = "";
			else if(security.value == 4) /* check 1x support for WPA2 */
				get_by_id("show_wpaAuth").style.display = "";
			else{
				get_by_id("show_wpaAuth").style.display = "none";
				form1.wpaAuth[1].checked = true;
			}
		}
		else{
			get_by_id("show_wpaAuth").style.display = "none";
			form1.wpaAuth[1].checked = true;
		}

		show_wpa_settings();
	}else if(security.value == 8 )	/* WIFI_SEC_WAPI */
	{
		get_by_id("setting_wapi").style.display = "";
		show_wapi_settings();
	}else if (security.value == 16 || security.value == 20 ){	/* WIFI_SEC_WPA3/WIFI_SEC_WPA2_WPA3_MIXED */
		get_by_id("setting_wpa").style.display = "";
		get_by_id("show_wpa3_cipher").style.display = "";
		if(support_11w){
			get_by_id("show_dotIEEE80211W").style.display = "";
			if(form1.dotIEEE80211W[1].checked == true)
				get_by_id("show_sha256").style.display = "";
		}

		//form1.wpa3ciphersuite_t.disabled = true;
		//form1.wpa3ciphersuite_t.checked = false;
		form1.wpa3ciphersuite_a.disabled = true;
		form1.wpa3ciphersuite_a.checked = true;

		if(security.value == 16){
			form1.dotIEEE80211W[2].checked = true;
			disableRadioGroup(form1.dotIEEE80211W);
			get_by_id("show_sha256").style.display = "none";
			form1.sha256[1].checked = true;
			disableRadioGroup(form1.sha256);
			if(support_1x == 1 && support_wpa3_1x == 1)
				get_by_id("show_wpaAuth").style.display = "";
			else{
				get_by_id("show_wpaAuth").style.display = "none";
				form1.wpaAuth[1].checked = true;
			}
		}

		if(security.value == 20){
			form1.dotIEEE80211W[1].checked = true;
			disableRadioGroup(form1.dotIEEE80211W);
			if(form1.dotIEEE80211W[1].checked == true)
				get_by_id("show_sha256").style.display = "";
			form1.sha256[1].checked = true;
			disableRadioGroup(form1.sha256);
			/*now WPA3-Mixed support Personal only*/
			get_by_id("show_wpaAuth").style.display = "none";
			form1.wpaAuth[1].checked = true;

		}

		show_wpa_settings();

		/*only show ASCII format for WPA3 mixed & WPA3 */
		if(form1.wpaAuth[1].checked == true){
			get_by_id("show_wpa_psk1").style.display = "none";
			form1.pskFormat.selectedIndex = 0;
		}
	}
	
	if (security.value == 0) {	/* WIFI_SEC_NONE */
		if (wlanMode != 1 && !is_wlan_qtn) {
			if(support_1x == 1 && enable_wpa2_and_wpa3_1x_only == 0)
				get_by_id("enable_8021x").style.display = "";
			else
				enable_1x.checked = false;
			if(enable_1x.checked){
				get_by_id("show_8021x_eap").style.display = "";
			}
			else {
				get_by_id("show_8021x_eap").style.display = "none";			
			}

			if(is_radius_2set == 1){
				if(enable_1x.checked){		
					get_by_id("show2_8021x_eap").style.display = "";
				}
				else {
					get_by_id("show2_8021x_eap").style.display = "none";			
				}
			}
		}
	}	
}

function setDefaultKeyValue(form, wlan_id)
{
  if (form.elements["length"+wlan_id].selectedIndex == 0) {
	if ( form.elements["format"+wlan_id].selectedIndex == 0) {
		form.elements["key"+wlan_id].maxLength = 5;
		form.elements["key"+wlan_id].value = "*****";
		
/*		
		form.elements["key1"+wlan_id].maxLength = 5;
		form.elements["key2"+wlan_id].maxLength = 5;
		form.elements["key3"+wlan_id].maxLength = 5;
		form.elements["key4"+wlan_id].maxLength = 5;
		form.elements["key1"+wlan_id].value = "*****";
		form.elements["key2"+wlan_id].value = "*****";
		form.elements["key3"+wlan_id].value = "*****";
		form.elements["key4"+wlan_id].value = "*****";
*/		
	}
	else {
		form.elements["key"+wlan_id].maxLength = 10;
		form.elements["key"+wlan_id].value = "**********";
		
/*		
		form.elements["key1"+wlan_id].maxLength = 10;
		form.elements["key2"+wlan_id].maxLength = 10;
		form.elements["key3"+wlan_id].maxLength = 10;
		form.elements["key4"+wlan_id].maxLength = 10;
		form.elements["key1"+wlan_id].value = "**********";
		form.elements["key2"+wlan_id].value = "**********";
		form.elements["key3"+wlan_id].value = "**********";
		form.elements["key4"+wlan_id].value = "**********";
*/
	}
  }
  else {
  	if ( form.elements["format"+wlan_id].selectedIndex == 0) {
		form.elements["key"+wlan_id].maxLength = 13;		
		form.elements["key"+wlan_id].value = "*************";		
/*		
		form.elements["key1"+wlan_id].maxLength = 13;
		form.elements["key2"+wlan_id].maxLength = 13;
		form.elements["key3"+wlan_id].maxLength = 13;
		form.elements["key4"+wlan_id].maxLength = 13;
		form.elements["key1"+wlan_id].value = "*************";
		form.elements["key2"+wlan_id].value = "*************";
		form.elements["key3"+wlan_id].value = "*************";
		form.elements["key4"+wlan_id].value = "*************";
*/		

	}
	else {
		form.elements["key"+wlan_id].maxLength = 26;
		form.elements["key"+wlan_id].value ="**************************";		
/*		
		form.elements["key1"+wlan_id].maxLength = 26;
		form.elements["key2"+wlan_id].maxLength = 26;
		form.elements["key3"+wlan_id].maxLength = 26;
		form.elements["key4"+wlan_id].maxLength = 26;
		form.elements["key1"+wlan_id].value ="**************************";
		form.elements["key2"+wlan_id].value ="**************************";
		form.elements["key3"+wlan_id].value ="**************************";
		form.elements["key4"+wlan_id].value ="**************************";
*/		
	}
  }
}
  
function a2hex(str) 
{
	var arr = [];
	for (var i = 0, l = str.length; i < l; i ++) {
		var hex = Number(str.charCodeAt(i)).toString(16);
		if(hex.length < 2)
			hex = '0' + hex
		arr.push(hex);
	}
	return arr.join('');
}

function updateWepFormat2(form)
{
	var index = form.wpaSSID.value;
	/*
	if (form.length0.selectedIndex == 0) {
		if ( form.format0.selectedIndex == 0)
		{
			document.formEncrypt.key0.value = decode64(_wepKeyValue[index]);
		}
		else{
			document.formEncrypt.key0.value = a2hex(decode64(_wepKeyValue[index]));
		}
	}
	else{
		if ( form.format0.selectedIndex == 0)
		{
			document.formEncrypt.key0.value = decode64(_wepKey128Value[index]);
		}
		else{
			document.formEncrypt.key0.value = a2hex(decode64(_wepKey128Value[index]));
		}
	}
	*/
	if (form.length0.selectedIndex == 0)
		document.formEncrypt.key0.value = decode64(_wepKeyValue[index]);
	else
		document.formEncrypt.key0.value = decode64(_wepKey128Value[index]);
}
  
function updateWepFormat(form, wlan_id)
{
	if (form.elements["length" + wlan_id].selectedIndex == 0) {
		form.elements["format" + wlan_id].options[0].text = 'ASCII (5 characters)';
		form.elements["format" + wlan_id].options[1].text = 'Hex (10 characters)';
		form.wepKeyLen[0].checked = true;
	}
	else {
		form.elements["format" + wlan_id].options[0].text = 'ASCII (13 characters)';
		form.elements["format" + wlan_id].options[1].text = 'Hex (26 characters)';
		form.wepKeyLen[1].checked = true;
	}
	//form.elements["format" + wlan_id].selectedIndex =  wep_key_fmt;
	// Mason Yu. TBD
	//form.elements["format" + wlan_id].selectedIndex =  0;
	
	//setDefaultKeyValue(form, wlan_id);
	updateWepFormat2(form);
}

function check_wepkey()
{
	form = document.formEncrypt;
	var keyLen;
	if (form.length0.selectedIndex == 0) {
  		if ( form.format0.selectedIndex == 0)
			keyLen = 5;
		else
			keyLen = 10;
	}
	else {
  	if ( form.format0.selectedIndex == 0)
		keyLen = 13;
	else
		keyLen = 26;
	}
	if (form.key0.value.length != keyLen) {
		alert('<% multilang(LANG_INVALID_LENGTH_OF_KEY_VALUE); %>');
		form.key0.focus();
		return 0;
	}
	if ( form.key0.value == "*****" ||
		form.key0.value == "**********" ||
		form.key0.value == "*************" ||
		form.key0.value == "**************************" ){
		if(wepkeyform==form.format0.value)
			return 1;
		else{
			alert("<% multilang(LANG_INVALID_KEY_VALUE); %>");
			form.key0.focus();
			return 0;
		}
	}
	
	if (form.format0.selectedIndex==0)
		return 1;
	
	for (var i=0; i<form.key0.value.length; i++) {
		if ( (form.key0.value.charAt(i) >= '0' && form.key0.value.charAt(i) <= '9') ||
			(form.key0.value.charAt(i) >= 'a' && form.key0.value.charAt(i) <= 'f') ||
			(form.key0.value.charAt(i) >= 'A' && form.key0.value.charAt(i) <= 'F') )
			continue;
	
		alert("<% multilang(LANG_INVALID_KEY_VALUE_IT_SHOULD_BE_IN_HEX_NUMBER_0_9_OR_A_F); %>");
		form.key0.focus();
		return 0;
	}
	
	return 1;
}

function check_rs()
{
	form = document.formEncrypt;
	if (checkHostIP(form.radiusIP, 1) == false) {
		return false;
	}
	if (form.radiusPort.value=="") {
		alert("<% multilang(LANG_RADIUS_SERVER_PORT_NUMBER_CANNOT_BE_EMPTY_IT_SHOULD_BE_A_DECIMAL_NUMBER_BETWEEN_1_65535); %>");
		form.radiusPort.focus();
		return false;
  	}
	if (validateKey(form.radiusPort.value)==0) {
		alert("<% multilang(LANG_RADIUS_SERVER_PORT_NUMBER_CANNOT_BE_EMPTY_IT_SHOULD_BE_A_DECIMAL_NUMBER_BETWEEN_1_65535); %>");
		form.radiusPort.focus();
		return false;
	}
        port = parseInt(form.radiusPort.value, 10);

 	if (port > 65535 || port < 1) {
		alert("<% multilang(LANG_INVALID_PORT_NUMBER_OF_RADIUS_SERVER_IT_SHOULD_BE_A_DECIMAL_NUMBER_BETWEEN_1_65535); %>");
		form.radiusPort.focus();
		return false;
  	}

	if(is_radius_2set == 1){
		if(form.radius2IP.value != "" && form.radius2IP.value != "0.0.0.0"){
			if (checkHostIP(form.radius2IP, 1) == false) {
				return false;
			}
			if (form.radius2Port.value=="") {
				alert("<% multilang(LANG_RADIUS_SERVER_PORT_NUMBER_CANNOT_BE_EMPTY_IT_SHOULD_BE_A_DECIMAL_NUMBER_BETWEEN_1_65535); %>");
				form.radius2Port.focus();
				return false;
		  	}
			if (validateKey(form.radius2Port.value)==0) {
				alert("<% multilang(LANG_RADIUS_SERVER_PORT_NUMBER_CANNOT_BE_EMPTY_IT_SHOULD_BE_A_DECIMAL_NUMBER_BETWEEN_1_65535); %>");
				form.radiusPort.focus();
				return false;
			}
		        port = parseInt(form.radius2Port.value, 10);

		 	if (port > 65535 || port < 1) {
				alert("<% multilang(LANG_INVALID_PORT_NUMBER_OF_RADIUS_SERVER_IT_SHOULD_BE_A_DECIMAL_NUMBER_BETWEEN_1_65535); %>");
				form.radius2Port.focus();
				return false;
		  	}
		}
		if(form.radius2IP.value == "")
			form.radius2IP.value = "0.0.0.0";
	}
	return true;
}

function isFirstSpaceOrLastSpace(str)
{
	if(str.length == 0)
		return false;
	if((str.charAt(0) == ' ') || (str.charAt(str.length-1) == ' '))
		return true;
	return false;
}

function saveChanges(obj)
{
  form = document.formEncrypt;
  wpaAuth = form.wpaAuth;
  if (form.security_method.value == 0) {	/* WIFI_SEC_NONE */
  	if(form.use1x.checked == true){
		if (check_rs() == false)
  			return false;
  	}
		
  	alert("<% multilang(LANG_WARNING_SECURITY_IS_NOT_SETTHIS_MAY_BE_DANGEROUS); %>");
  }
  else if (form.security_method.value == 1) {	/* WIFI_SEC_WEP */
  	if (form.use1x.checked == false) {
  		if (check_wepkey() == false)
  			return false;
  	}
  	else {
  		if (check_rs() == false)
  			return false;
  	}
	if (wps20 && wps20_use_version!=0 && form.wpaSSID.value==0)
		alert("<% multilang(LANG_INFO_WPS_WILL_BE_DISABLED_WHEN_USING_WEP); %>");
  }
  else if (form.security_method.value == 2 || form.security_method.value == 4 || form.security_method.value == 6 /* WPA or WPA2 or WPA2 mixed */
		|| form.security_method.value == 16 || form.security_method.value == 20) {	/* WPA3 or WPA3 mixed */
    if (form.security_method.value == 2) {	/* WIFI_SEC_WPA */
    	if(form.ciphersuite_t.checked == false && form.ciphersuite_a.checked == false )
		{
			alert("<% multilang(LANG_WPA_CIPHER_SUITE_CAN_NOT_BE_EMPTY); %>");
			return false;
		}
		if (form.isNmode.value == 1 && form.ciphersuite_t.checked == true && form.ciphersuite_a.checked == true)
		{
			alert("<% multilang(LANG_CAN_NOT_SELECT_TKIP_AND_AES_IN_THE_SAME_TIME); %>");
			return false;				
		}
		if (wps20 && wps20_use_version!=0 && form.wpaSSID.value==0)
			alert("<% multilang(LANG_INFO_WPS_WILL_BE_DISABLED_WHEN_USING_WPA_ONLY); %>");
    }
    
    if (form.security_method.value == 4) {	/* WIFI_SEC_WPA2 */
    	if(form.wpa2ciphersuite_t.checked == false && form.wpa2ciphersuite_a.checked == false )
		{
			alert("<% multilang(LANG_WPA2_CIPHER_SUITE_CAN_NOT_BE_EMPTY); %>");
			return false;
		}
		if (form.isNmode.value == 1 && form.wpa2ciphersuite_t.checked == true && form.wpa2ciphersuite_a.checked == true)
		{
			alert("<% multilang(LANG_CAN_NOT_SELECT_TKIP_AND_AES_IN_THE_SAME_TIME); %>");
			return false;				
		}
		if (form.wpa2ciphersuite_t.checked == true) {
			if (wps20 && wps20_use_version!=0 && form.wpaSSID.value==0 && form.wpa2ciphersuite_a.checked == false)
				alert("<% multilang(LANG_INFO_WPS_WILL_BE_DISABLED_WHEN_USING_TKIP_ONLY); %>");
		}
    }

	if (form.security_method.value == 6) {	/* WIFI_SEC_WPA2_MIXED */
		if(wlanMode == 1 && ((form.ciphersuite_t.checked == true && form.ciphersuite_a.checked == true)
			|| (form.wpa2ciphersuite_t.checked == true && form.wpa2ciphersuite_a.checked == true)))
		{
			alert("<% multilang(LANG_IN_THE_CLIENT_MODE_YOU_CAN_T_SELECT_TKIP_AND_AES_IN_THE_SAME_TIME); %>");
			return false;				
		}
    	if(form.ciphersuite_t.checked == false && form.ciphersuite_a.checked == false )
		{
			alert("<% multilang(LANG_WPA_CIPHER_SUITE_CAN_NOT_BE_EMPTY); %>");
			return false;
		}
		if(form.wpa2ciphersuite_t.checked == false && form.wpa2ciphersuite_a.checked == false )
		{
			alert("<% multilang(LANG_WPA2_CIPHER_SUITE_CAN_NOT_BE_EMPTY); %>");
			return false;
		}
		
		if (wps20 && wps20_use_version!=0 && form.wpaSSID.value==0 && form.ciphersuite_t.checked == true && form.wpa2ciphersuite_t.checked == true
			&& form.ciphersuite_a.checked == false && form.wpa2ciphersuite_a.checked == false)
			alert("<% multilang(LANG_INFO_WPS_WILL_BE_DISABLED_WHEN_USING_TKIP_ONLY); %>");
    }
	
	if(wpaAuth[0].checked){
		if(check_rs()==false)
			return false;
	}
	var str = form.pskValue.value;
	if (form.pskFormat.selectedIndex==1) {
		if (str.length != 64) {
			alert('<% multilang(LANG_PRE_SHARED_KEY_VALUE_SHOULD_BE_64_CHARACTERS); %>');
			form.pskValue.focus();
			return false;
		}
		takedef = 0;
		if (defPskFormat == 1 && defPskLen == 64) {
			for (var i=0; i<64; i++) {
    				if ( str.charAt(i) != '*')
					break;
			}
			if (i == 64 )
				takedef = 1;
  		}
  		
		if (takedef == 0) {
			for (var i=0; i<str.length; i++) {
    				if ( (str.charAt(i) >= '0' && str.charAt(i) <= '9') ||
					(str.charAt(i) >= 'a' && str.charAt(i) <= 'f') ||
					(str.charAt(i) >= 'A' && str.charAt(i) <= 'F') )
					continue;
				alert("<% multilang(LANG_INVALID_PRE_SHARED_KEY_VALUE_IT_SHOULD_BE_IN_HEX_NUMBER_0_9_OR_A_F); %>");
				form.pskValue.focus();
				return false;
  			}
		}
	}
	else {
		if ( (form.security_method.value > 1) && wpaAuth[1].checked ) {
			if (str.length < 8) {
				alert('<% multilang(LANG_PRE_SHARED_KEY_VALUE_SHOULD_BE_SET_AT_LEAST_8_CHARACTERS); %>');
				form.pskValue.focus();
				return false;
			}
			if (str.length > 63) {
				alert('<% multilang(LANG_PRE_SHARED_KEY_VALUE_SHOULD_BE_LESS_THAN_64_CHARACTERS); %>');
				form.pskValue.focus();
				return false;
			}
			if (checkPrintableString(form.pskValue.value) == 0) {
				alert('<% multilang(LANG_INVALID_PRE_SHARED_KEY); %>');
				form.pskValue.focus();
				return false;
			}
			if(isFirstSpaceOrLastSpace(str)){
				alert('<% multilang(LANG_WARNING_SOME_CLIENTS_MAY_BE_UNABLE_TO_CONNECT_VIA_WPS_WHEN_THERE_IS_A_BLANK_SPACE_DIRECTLY_PRECEDING_OR_FOLLOWING_THE_PASSWORD); %>');
			}
		}
	}
	if (wpa3_disable_wps && form.security_method.value == 16)
		alert("<% multilang(LANG_INFO_WPS_WILL_BE_DISABLED_WHEN_USING_WPA3_ONLY); %>");
  }

	form.encodekey0.value = encode64(form.key0.value);
	form.key0.disabled=true;

  	form.encodepskValue.value = encode64(form.pskValue.value);
	form.pskValue.disabled=true;

  	form.encoderadiusPass.value = encode64(form.radiusPass.value);
	form.radiusPass.disabled=true;

  	form.encoderadius2Pass.value = encode64(form.radius2Pass.value);
	form.radius2Pass.disabled=true;

	form.wlan6gSupport.value = wlan6gSupport;
   obj.isclick = 1;
   postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
   return true;
}


function postSecurity(encrypt, enable1X, wpaAuth, wpaPSKFormat, wpaPSK, wpaGroupRekeyTime, rsPort, rsIpAddr, rsPassword, rs2Port, rs2IpAddr, rs2Password, uCipher, wpa2uCipher, wepAuth, wepLen, wepKeyFormat, dotIeee80211w, sh256, sae_pwe, wepKey, wepKey128)
{	
	document.formEncrypt.security_method.value = encrypt;
	document.formEncrypt.pskFormat.value = wpaPSKFormat;
	document.formEncrypt.pskValue.value = decode64(wpaPSK);				
	document.formEncrypt.gk_rekey.value = wpaGroupRekeyTime;
	document.formEncrypt.radiusIP.value = rsIpAddr;
	document.formEncrypt.radiusPort.value = rsPort;
	document.formEncrypt.radiusPass.value = decode64(rsPassword);

	if(is_radius_2set == 1)
	{
		document.formEncrypt.radius2IP.value = rs2IpAddr;
		document.formEncrypt.radius2Port.value = rs2Port;
		document.formEncrypt.radius2Pass.value = decode64(rs2Password);
	}
		
	if (document.formEncrypt.wepKeyLen && wepLen > 0)
		document.formEncrypt.wepKeyLen[wepLen-1].checked = true;
	
	if (enable1X==1)
		document.formEncrypt.use1x.checked = true;		
	else
		document.formEncrypt.use1x.checked = false;
	document.formEncrypt.wpaAuth[wpaAuth-1].checked = true;	
	
	document.formEncrypt.ciphersuite_t.checked = false;
	document.formEncrypt.ciphersuite_a.checked = false;
	if ( uCipher == 1 )
		document.formEncrypt.ciphersuite_t.checked = true;
	if ( uCipher == 2 )
		document.formEncrypt.ciphersuite_a.checked = true;
	if ( uCipher == 3 ) {
		document.formEncrypt.ciphersuite_t.checked = true;
		document.formEncrypt.ciphersuite_a.checked = true;
	}
	
	document.formEncrypt.wpa2ciphersuite_t.checked = false;
	document.formEncrypt.wpa2ciphersuite_a.checked = false;	
	if ( wpa2uCipher == 1 )
		document.formEncrypt.wpa2ciphersuite_t.checked = true;
	if ( wpa2uCipher == 2 )
		document.formEncrypt.wpa2ciphersuite_a.checked = true;
	if ( wpa2uCipher == 3 ) {
		document.formEncrypt.wpa2ciphersuite_t.checked = true;
		document.formEncrypt.wpa2ciphersuite_a.checked = true;
	}	

	document.formEncrypt.auth_type[wepAuth].checked = true;
	
	if ( wepLen == 0 )
		document.formEncrypt.length0.value = 1;
	else
		document.formEncrypt.length0.value = wepLen;
	
	if(wepLen==2){
		document.formEncrypt.key0.value = decode64(wepKey128);
	}
	else{
		document.formEncrypt.key0.value = decode64(wepKey);
	}
	document.formEncrypt.format0.value = wepKeyFormat+1;
	wepkeyform = wepKeyFormat+1;
	if(support_11w){
		document.formEncrypt.dotIEEE80211W[dotIeee80211w].checked = true;
		document.formEncrypt.sha256[sh256].checked = true;
	}
	if(support_wpa3_h2e){
		document.formEncrypt.wpa3_sae_pwe[sae_pwe==1? 1:0].checked = true;
	}
	show_authentication(0);

	
        defPskLen = document.formEncrypt.pskValue.value.length;
	defPskFormat = document.formEncrypt.pskFormat.selectedIndex;
	updateWepFormat(document.formEncrypt, 0);
}

var backhaulIndex0 = <% checkWrite("backhaulIndexQuery_0"); %>;
var backhaulIndex1 = <% checkWrite("backhaulIndexQuery_1"); %>;

function SSIDSelected(index)
{
	if (document.formEncrypt.wlanDisabled.value == "ON") {
		disableTextField(document.formEncrypt.wpaSSID);
		disableTextField(document.formEncrypt.security_method);
		disableCheckBox(document.formEncrypt.use1x);
		disableRadioGroup(document.formEncrypt.auth_type);
		disableTextField(document.formEncrypt.length0);
		disableTextField(document.formEncrypt.format0);
		disableTextField(document.formEncrypt.key0);
		disableRadioGroup(document.formEncrypt.wpaAuth);
		disableRadioGroup(document.formEncrypt.dotIEEE80211W);
		disableRadioGroup(document.formEncrypt.sha256);
		disableCheckBox(document.formEncrypt.ciphersuite_t);
		disableCheckBox(document.formEncrypt.ciphersuite_a);
		disableCheckBox(document.formEncrypt.wpa2ciphersuite_t);
		disableCheckBox(document.formEncrypt.wpa2ciphersuite_a);
		disableTextField(document.formEncrypt.pskFormat);
		disableTextField(document.formEncrypt.pskValue);
		disableRadioGroup(document.formEncrypt.wapiAuth);
		disableTextField(document.formEncrypt.wapiPskFormat);
		disableTextField(document.formEncrypt.wapiPskValue);
		disableRadioGroup(document.formEncrypt.wepKeyLen);
		disableTextField(document.formEncrypt.radiusIP);
		disableTextField(document.formEncrypt.radiusPort);
		disableTextField(document.formEncrypt.radiusPass);
		//if(is_radius_2set == 1)
		{
			disableTextField(document.formEncrypt.radius2IP);
			disableTextField(document.formEncrypt.radius2Port);
			disableTextField(document.formEncrypt.radius2Pass);
		}
		disableCheckBox(document.formEncrypt.uselocalAS);
		disableTextField(document.formEncrypt.wapiASIP);
		disableTextField(document.formEncrypt.save);
	}
	if (ssid_num == 0)
		return;
	if (index != 0 && ((backhaulIndex0 == index && wlan_idx == 0) || (backhaulIndex1 == index && wlan_idx == 1))) {
		document.formEncrypt.security_method.disabled = true;
		document.formEncrypt.pskValue.disabled = true;
	} else {
		document.formEncrypt.security_method.disabled = false;
		document.formEncrypt.pskValue.disabled = false;
	}
	wlanMode = _wlan_mode[index];
	document.formEncrypt.isNmode.value = _wlan_isNmode[index];
	postSecurity(_encrypt[index], _enable1X[index],
		_wpaAuth[index], _wpaPSKFormat[index], _wpaPSK[index],
		_wpaGroupRekeyTime[index],
		_rsPort[index], _rsIpAddr[index], _rsPassword[index],
		_rs2Port[index], _rs2IpAddr[index], _rs2Password[index],
		_uCipher[index], _wpa2uCipher[index], _wepAuth[index],
		_wepLen[index], _wepKeyFormat[index], _dotIEEE80211W[index], _sha256[index],
		_wpa3_sae_pwe[index],
		_wepKeyValue[index], _wepKey128Value[index]);
}

function show_password(id)
{
	var x= document.formEncrypt.pskValue;
	if(id==1){
		x= document.formEncrypt.pskValue;
}
	else if(id==2){
		x= document.formEncrypt.wapiPskValue;
	}
	else if(id==3){
		x= document.formEncrypt.radiusPass;
	}
	else if(id==4){
		x= document.formEncrypt.radius2Pass;
	}
	else if(id==5){
		x= document.formEncrypt.key0;
	}
    if (x.type == "password") {
        x.type = "text";
    } else {
        x.type = "password";
    }
}

</script>

</head>

<body onload="SSIDSelected(0);">
<div class="intro_main ">
	<p class="intro_title"><% multilang(LANG_WLAN_SECURITY_SETTINGS); %></p>
	<p class="intro_content"><% multilang(LANG_PAGE_DESC_WLAN_SECURITY_SETTING); %></p>
</div>

<form action=/boaform/admin/formWlEncrypt method=POST name="formEncrypt">
<div id="wlan_security_table" style="display:none" class="data_common data_common_notitle">
	<table>
		<input type=hidden name="wlanDisabled" value=<% wlanStatus(); %>>
		<input type=hidden name="isNmode" value=0 >    
		<tr>
			<th width="30%"><% multilang(LANG_SSID); %> <% multilang(LANG_TYPE); %>:</th>
			<td width="70%">
				<select name=wpaSSID onChange="SSIDSelected( this.value )">
				<% SSID_select(); %>
				</select>
			</td>
		</tr>    
	</table>
	<table>  
		<tr>
			<th width="30%"><% multilang(LANG_ENCRYPTION); %>:&nbsp;</th>
			<td width="70%">
				<select size="1" id="security_method" name="security_method" onChange="show_authentication(1)">
					<% checkWrite("wifiSecurity"); %> 
				</select>
			</td>
		</tr>   
		<tr id="enable_8021x" style="display:none">
			<th width="30%">802.1x <% multilang(LANG_AUTHENTICATION); %>:</th>
			<td width="70%">
				<input type="checkbox" id="use1x" name="use1x" value="ON" onClick="show_8021x_settings()">
			</td>
		</tr>
		<tr id="show_wep_auth" style="display:none">
			<th width="30%"><% multilang(LANG_AUTHENTICATION); %>:</th>
			<td width="70%">
				<input name="auth_type" type=radio value="open"><% multilang(LANG_OPEN_SYSTEM); %>
				<input name="auth_type" type=radio value="shared"><% multilang(LANG_SHARED_KEY); %>
				<input name="auth_type" type=radio value="both"><% multilang(LANG_AUTO); %>
			</td>
		</tr>
	</table>
  
    <table id="setting_wep" style="display:none">	
    	<input type="hidden" name="wepEnabled" value="ON" checked>
		<tr>
			<th width="30%"><% multilang(LANG_KEY_LENGTH); %>:</th>		
			<td width="70%">
				<select size="1" name="length0" id="key_length" onChange="updateWepFormat(document.formEncrypt, 0)">	
					<option value=1> 64-bit</option>
					<option value=2>128-bit</option>
				</select>
			</td>
		</tr>
		<tr>
			<th width="30%"><% multilang(LANG_KEY_FORMAT); %>:</th>
			<td width="70%">
				<select id="key_format" name="format0" onChange="updateWepFormat2(document.formEncrypt)">
					<option value="1">ASCII</option>
					<option value="2">Hex</option>					
				</select>
			</td>
		</tr>
		<tr>
			<th width="30%"><% multilang(LANG_ENCRYPTION_KEY); %>:</th>
			<td width="70%">
				<input type="password" id="key" name="key0" maxlength="26" size="26" value="">
				<input type="checkbox" onclick="show_password(5)" value=0>Show Password
			</td>
		</tr> 
	</table>				     

	<table id="setting_wpa" style="display:none">
		<tr id="show_wpaAuth">
			<th width="30%"><% multilang(LANG_AUTHENTICATION_MODE); %>:</th>
			<td width="70%">
				<input name="wpaAuth" type="radio" value="eap" onClick="show_wpa_settings()">Enterprise (RADIUS)
				<input name="wpaAuth" type="radio" value="psk" onClick="show_wpa_settings()">Personal (Pre-Shared Key)
			</td>  
		</tr>
		<tr id="show_wpa3_sae_pwe" style="display:none">
			<th width="30%"><% multilang(LANG_H2E); %>:</th>
			<td width="70%">
				<input name="wpa3_sae_pwe" type="radio" value="2">Capable
				<input name="wpa3_sae_pwe" type="radio" value="1">Required
			</td>
		</tr>
		<tr id="show_dotIEEE80211W" style="display:none">
			<th width="30%"><% multilang(LANG_IEEE_802_11W); %>:</th>
			<td width="70%">
				<input name="dotIEEE80211W" type="radio" value="0" onClick="show_sha256_settings()">None
				<input name="dotIEEE80211W" type="radio" value="1" onClick="show_sha256_settings()">Capable
				<input name="dotIEEE80211W" type="radio" value="2" onClick="show_sha256_settings()">Required
			</td>
		</tr>

		<tr id="show_sha256" style="display:none">
			<th width="30%"><% multilang(LANG_SHA256); %>:</th>
			<td width="70%">
				<input name="sha256" type="radio" value="0">Disable
				<input name="sha256" type="radio" value="1">Enable
			</td>
		</tr>
		<tr id="show_wpa_cipher" style="display:none">
			<th width="30%">WPA <% multilang(LANG_CIPHER_SUITE); %>:</th>
			<td width="70%">
				<input type="checkbox" name="ciphersuite_t" value=1>TKIP&nbsp;
				<input type="checkbox" name="ciphersuite_a" value=1>AES		
			</td>
		</tr>
		<tr id="show_wpa2_cipher" style="display:none">
			<th width="30%">WPA2 <% multilang(LANG_CIPHER_SUITE); %>:</th>
			<td width="70%">
				<input type="checkbox" name="wpa2ciphersuite_t" value=1>TKIP&nbsp;
				<input type="checkbox" name="wpa2ciphersuite_a" value=1>AES
			</td>
		</tr>
		<tr id="show_wpa3_cipher" style="display:none">
			<th width="30%"><% multilang(LANG_CIPHER_SUITE); %>:</th>
			<td width="70%">
				<!--<input type="checkbox" name="wpa3ciphersuite_t" value=1>TKIP&nbsp;-->
				<input type="checkbox" name="wpa3ciphersuite_a" value=1>AES
			</td>
		</tr>
		<tr id="show_wpa_gk_rekey" style="display:none">
			<th width="30%"><% multilang(LANG_GROUP_KEY_UPDATE_TIMER); %>:</th>
			<td width="70%"><input type="text" name="gk_rekey" size="32" maxlength="10" value="">
			</td>
		</tr>
		<tr id="show_wpa_psk1" style="display:none">				
			<th width="30%"><% multilang(LANG_PRE_SHARED_KEY_FORMAT); %>:</th>
			<td width="70%">
				<select id="psk_fmt" name="pskFormat" onChange="">
					<option value="0">Passphrase</option>
					<option value="1">HEX (64 characters)</option>
				</select>
			</td>
		</tr>
		<tr id="show_wpa_psk2" style="display:none">
			<th width="30%"><% multilang(LANG_PRE_SHARED_KEY); %>:</th>
			<td width="70%">
			<input type="password" name="pskValue" id="wpapsk" size="32" maxlength="64" value="">
			<input type="checkbox" onclick="show_password(1)" value=0>Show Password
			</td>
		</tr>
 	</table>
			
	<table id="setting_wapi" style="display:none"> 
		<tr>
			<th width="30%"><% multilang(LANG_AUTHENTICATION_MODE); %>:</th>
			<td width="70%">
			        <input name="wapiAuth" type="radio" value="eap" onClick="show_wapi_settings()">Enterprise (AS Server)
			        <input name="wapiAuth" type="radio" value="psk" onClick="show_wapi_settings()">Personal (Pre-Shared Key)
			</td>
		</tr>
		<tr id="show_wapi_psk1" style="display:none">
			<th width="30%"><% multilang(LANG_PRE_SHARED_KEY_FORMAT); %>:</th>
			<td width="70%">
			<select id="wapi_psk_fmt" name="wapiPskFormat" onChange="">
			        <option value="0">Passphrase</option>
			        <option value="1">HEX (64 characters)</option>
			        </select>
			</td>
		</tr>
		<tr id="show_wapi_psk2" style="display:none">
			<th width="30%"><% multilang(LANG_PRE_SHARED_KEY); %>:</th>
			<td width="70%">
			<input type="password" name="wapiPskValue" id="wapipsk" size="32" maxlength="64" value="">
			<input type="checkbox" onclick="show_password(2)" value=0>Show Password
			</td>
		</tr>
	</table>
	
	<table id="show_1x_wep" style="display:none">	
		<tr>	
			<th width="30%"><% multilang(LANG_KEY_LENGTH); %>:</th>
			<td width="70%">
				<input name="wepKeyLen" type="radio" value="wep64">64 Bits
				<input name="wepKeyLen" type="radio" value="wep128">128 Bits
			</td>
		</tr>
	</table>

	<table id="show_8021x_eap" style="display:none">
		<tr>
			<th width="30%">RADIUS <% multilang(LANG_SERVER); %>:</th>
			<td width="70%">
			<% multilang(LANG_IP_ADDRESS); %>:<input id="radius_ip" name="radiusIP" size="16" maxlength="15" value="0.0.0.0">
			<% multilang(LANG_PORT); %>:<input type="text" id="radius_port" name="radiusPort" size="4" maxlength="5" value="1812">
			<% multilang(LANG_PASSWORD); %>:<input type="password" id="radius_pass" name="radiusPass" size="20" maxlength="64" value="12345"><input type="checkbox" onclick="show_password(3)" value=0>Show Password
			</td>
		</tr>

	</table>
	<table id="show2_8021x_eap" style="display:none">
		<tr>
			<th width="30%">Backup RADIUS <% multilang(LANG_SERVER); %>:</th>
			<td width="70%">
			<% multilang(LANG_IP_ADDRESS); %>:<input id="radius2_ip" name="radius2IP" size="16" maxlength="15" value="0.0.0.0">
			<% multilang(LANG_PORT); %>:<input type="text" id="radius2_port" name="radius2Port" size="4" maxlength="5" value="1812">
			<% multilang(LANG_PASSWORD); %>:<input type="password" id="radius2_pass" name="radius2Pass" size="20" maxlength="64" value="12345"><input type="checkbox" onclick="show_password(4)" value=0>Show Password
			</td>
		</tr>
	</table>								

	<table id="show_8021x_wapi" style="display:none">     
		<tr id="show_8021x_wapi_local_as" style="">
			<th width="30%"><% multilang(LANG_USE_LOCAL_AS_SERVER); %>:</th>
			<td width="70%">
			<input type="checkbox" id="uselocalAS" name="uselocalAS" value="ON" onClick="show_wapi_ASip()">
			</td>
		</tr>
		<tr>
			<th width="30%">AS <% multilang(LANG_SERVER); %> <% multilang(LANG_IP_ADDRESS); %>:</th>
			<td width="70%"><input id="wapiAS_ip" name="wapiASIP" size="16" maxlength="15" value="0.0.0.0"></td>
		</tr>
	</table>
</div> 		
<div class="btn_ctl">       
	<input type="hidden" name="wlan_idx" value=<% checkWrite("wlan_idx"); %>>
	<input type="hidden" name="wlan6gSupport" value="">
	<input type="hidden" value="/admin/wlwpa.asp" name="submit-url">
	<input type="hidden" name="encodekey0" value="">
	<input type="hidden" name="encodepskValue" value="">
	<input type="hidden" name="encoderadiusPass" value="">
	<input type="hidden" name="encoderadius2Pass" value="">
	<input type=submit name="save" value="<% multilang(LANG_APPLY_CHANGES); %>" onClick="return saveChanges(this)" class="link_bg">&nbsp;
	<input type="hidden" name="postSecurityFlag" value="">
</div>
<script>
	<% initPage("wlwpa_mbssid"); %>
	<% checkWrite("wpsVer"); %>
	<% checkWrite("wpa3_disable_wps"); %>
	show_authentication(0);
	defPskLen = document.formEncrypt.pskValue.value.length;
	defPskFormat = document.formEncrypt.pskFormat.selectedIndex;
	updateWepFormat(document.formEncrypt, 0);
</script>
</form>
<br><br>
</body>
</html>
