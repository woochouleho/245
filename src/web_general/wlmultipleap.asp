<% SendWebHeadStr();%>
<title>Multiple AP</title>
<style>
.on {display:on}
.off {display:none}
</style>
<script>
var wlan_idx=<% checkWrite("wlan_idx"); %> ;
var WiFiTest=<% getInfo("WiFiTest"); %> ;
var val;
var bandIdx=new Array();
var virtual_bandIdx=new Array();
var virtual_wmm_enable=new Array();
var virtual_wlan_enable=new Array();
var virtual_access=new Array();
var virtual_hidessid=new Array();
var wlan_encrypt=new Array();
var aclist_index;
var mssid_num=<% checkWrite("wlan_mssid_num"); %>;
var fon=<%checkWrite("wlan_fon");%>;
var chanwid=<% getInfo("chanwid"); %>;
var wlan_support_11ax=<% checkWrite("wlan_support_11ax"); %>;
var wlan_wifi5_wifi6_comp=<% checkWrite("wlan_wifi5_wifi6_comp"); %>;
var wlan_support_deny_legacy=<% checkWrite("wlan_support_deny_legacy"); %>;
var isCMCCSupport = <% checkWrite("isCMCCSupport"); %>;

var easymesh_enabled=<% checkWrite("easymesh_enabled"); %> && (64 == <% getVirtualIndex("multiap_bss_type", "1"); %>);

val =  <% checkWrite("mband"); %>;
if (val > 0) 
	val = val-1;
bandIdx[wlan_idx] = val;

if (mssid_num >= 1) {
	val = <% getVirtualIndex("band", "1"); %>;
	if (val > 0)
		val = val-1;
	virtual_bandIdx[1] = val;

	val = <% getVirtualIndex("wmmEnabled", "1"); %>;
	virtual_wmm_enable[1] = val;

	val = <% getVirtualIndex("wlanDisabled", "1"); %>;
	if (val) val = 0;
	else val = 1;
	virtual_wlan_enable[1] = val;

	val = <% getVirtualIndex("wlanAccess", "1"); %>;
	virtual_access[1] = val;

	val = <% getVirtualIndex("hidessid", "1"); %>;
	virtual_hidessid[1] = val;

	wlan_encrypt[1]=<% getVirtualIndex("encrypt","1"); %>;
}
if (mssid_num >= 2) {
	val = <% getVirtualIndex("band", "2"); %>;
	if (val > 0)
		val = val-1;
	virtual_bandIdx[2] = val;

	val = <% getVirtualIndex("wmmEnabled", "2"); %>;
	virtual_wmm_enable[2] = val;

	val = <% getVirtualIndex("wlanDisabled", "2"); %>;
	if (val) val = 0;
	else val = 1;
	virtual_wlan_enable[2] = val;

	val = <% getVirtualIndex("wlanAccess", "2"); %>;
	virtual_access[2] = val;

	val = <% getVirtualIndex("hidessid", "2"); %>;
	virtual_hidessid[2] = val;

	wlan_encrypt[2]=<% getVirtualIndex("encrypt","2"); %>;
}
if (mssid_num >= 3) {
	val = <% getVirtualIndex("band", "3"); %>;
	if (val > 0)
		val = val-1;
	virtual_bandIdx[3] = val;

	val = <% getVirtualIndex("wmmEnabled", "3"); %>;
	virtual_wmm_enable[3] = val;

	val = <% getVirtualIndex("wlanDisabled", "3"); %>;
	if (val) val = 0;
	else val = 1;
	virtual_wlan_enable[3] = val;

	val = <% getVirtualIndex("wlanAccess", "3"); %>;
	virtual_access[3] = val;

	val = <% getVirtualIndex("hidessid", "3"); %>;
	virtual_hidessid[3] = val;

	wlan_encrypt[3]=<% getVirtualIndex("encrypt","3"); %>;
}
if (mssid_num >= 4) {
	val = <% getVirtualIndex("band", "4"); %>;
	if (val > 0)
		val = val-1;
	virtual_bandIdx[4] = val;

	val = <% getVirtualIndex("wmmEnabled", "4"); %>;
	virtual_wmm_enable[4] = val;

	val = <% getVirtualIndex("wlanDisabled", "4"); %>;
	if (val) val = 0;
	else val = 1;
	virtual_wlan_enable[4] = val;

	val = <% getVirtualIndex("wlanAccess", "4"); %>;
	virtual_access[4] = val;

	val = <% getVirtualIndex("hidessid", "4"); %>;
	virtual_hidessid[4] = val;

	wlan_encrypt[4]=<% getVirtualIndex("encrypt","4"); %>;
}

var rate_mask = [255,1,1,1,1,2,2,2,2,2,2,2,2,4,4,4,4,4,4,4,4,8,8,8,8,8,8,8,8,16,16,16,16,16,16,16,16,32,32,32,32,32,32,32,32];
var rate_name =["Auto","1M","2M","5.5M","11M","6M","9M","12M","18M","24M","36M","48M","54M", "MCS0", "MCS1"
				, "MCS2", "MCS3", "MCS4", "MCS5", "MCS6", "MCS7", "MCS8", "MCS9", "MCS10", "MCS11", "MCS12", "MCS13", "MCS14", "MCS15"
				, "MCS16", "MCS17", "MCS18", "MCS19", "MCS20", "MCS21", "MCS22", "MCS23"
				, "MCS24", "MCS25", "MCS26", "MCS27", "MCS28", "MCS29", "MCS30", "MCS31"];
var vht_rate_name=["NSS1-MCS0","NSS1-MCS1","NSS1-MCS2","NSS1-MCS3","NSS1-MCS4",
		"NSS1-MCS5","NSS1-MCS6","NSS1-MCS7","NSS1-MCS8","NSS1-MCS9",
		"NSS2-MCS0","NSS2-MCS1","NSS2-MCS2","NSS2-MCS3","NSS2-MCS4",
		"NSS2-MCS5","NSS2-MCS6","NSS2-MCS7","NSS2-MCS8","NSS2-MCS9",
		"NSS3-MCS0","NSS3-MCS1","NSS3-MCS2","NSS3-MCS3","NSS3-MCS4",
		"NSS3-MCS5","NSS3-MCS6","NSS3-MCS7","NSS3-MCS8","NSS3-MCS9",
		"NSS4-MCS0","NSS4-MCS1","NSS4-MCS2","NSS4-MCS3","NSS4-MCS4",
		"NSS4-MCS5","NSS4-MCS6","NSS4-MCS7","NSS4-MCS8","NSS4-MCS9"];
var he_rate_name=["HE-NSS1-MCS0","HE-NSS1-MCS1","HE-NSS1-MCS2","HE-NSS1-MCS3","HE-NSS1-MCS4",
		"HE-NSS1-MCS5","HE-NSS1-MCS6","HE-NSS1-MCS7","HE-NSS1-MCS8","HE-NSS1-MCS9","HE-NSS1-MCS10","HE-NSS1-MCS11",
		"HE-NSS2-MCS0","HE-NSS2-MCS1","HE-NSS2-MCS2","HE-NSS2-MCS3","HE-NSS2-MCS4",
		"HE-NSS2-MCS5","HE-NSS2-MCS6","HE-NSS2-MCS7","HE-NSS2-MCS8","HE-NSS2-MCS9","HE-NSS2-MCS10","HE-NSS2-MCS11",
		"HE-NSS4-MCS0","HE-NSS4-MCS1","HE-NSS4-MCS2","HE-NSS4-MCS3","HE-NSS4-MCS4",
		"HE-NSS4-MCS5","HE-NSS4-MCS6","HE-NSS4-MCS7","HE-NSS4-MCS8","HE-NSS4-MCS9","HE-NSS4-MCS10","HE-NSS4-MCS11",
		"HE-NSS8-MCS0","HE-NSS8-MCS1","HE-NSS8-MCS2","HE-NSS8-MCS3","HE-NSS8-MCS4",
		"HE-NSS8-MCS5","HE-NSS8-MCS6","HE-NSS8-MCS7","HE-NSS8-MCS8","HE-NSS8-MCS9","HE-NSS8-MCS10","HE-NSS8-MCS11"];


//--------------------Multi AP----------------------//
var enable_vlan =<% checkWrite("map_enable_vlan"); %>;
var default_sec_vid =<% checkWrite("map_default_secondary_vid"); %>;
var map_fronthaul = new Array(10);
	map_fronthaul[0] = <% checkWrite("map_fronthaul_bss_00"); %>;
	map_fronthaul[1] = <% checkWrite("map_fronthaul_bss_01"); %>;
	map_fronthaul[2] = <% checkWrite("map_fronthaul_bss_02"); %>;
	map_fronthaul[3] = <% checkWrite("map_fronthaul_bss_03"); %>;
	map_fronthaul[4] = <% checkWrite("map_fronthaul_bss_04"); %>;
	map_fronthaul[5] = <% checkWrite("map_fronthaul_bss_10"); %>;
	map_fronthaul[6] = <% checkWrite("map_fronthaul_bss_11"); %>;
	map_fronthaul[7] = <% checkWrite("map_fronthaul_bss_12"); %>;
	map_fronthaul[8] = <% checkWrite("map_fronthaul_bss_13"); %>;
	map_fronthaul[9] = <% checkWrite("map_fronthaul_bss_14"); %>;
var map_ssid = new Array(10).fill(0);
	map_ssid[0] = "<% checkWrite("interface_info_query_00"); %>";
	map_ssid[1] = "<% checkWrite("interface_info_query_01"); %>";
	map_ssid[2] = "<% checkWrite("interface_info_query_02"); %>";
	map_ssid[3] = "<% checkWrite("interface_info_query_03"); %>";
	map_ssid[4] = "<% checkWrite("interface_info_query_04"); %>";
	map_ssid[5] = "<% checkWrite("interface_info_query_10"); %>";
	map_ssid[6] = "<% checkWrite("interface_info_query_11"); %>";
	map_ssid[7] = "<% checkWrite("interface_info_query_12"); %>";
	map_ssid[8] = "<% checkWrite("interface_info_query_13"); %>";
	map_ssid[9] = "<% checkWrite("interface_info_query_14"); %>";
var map_vid = new Array(10).fill(0);
	map_vid[0] = <% checkWrite("map_vid_00"); %>;
	map_vid[1] = <% checkWrite("map_vid_01"); %>;
	map_vid[2] = <% checkWrite("map_vid_02"); %>;
	map_vid[3] = <% checkWrite("map_vid_03"); %>;
	map_vid[4] = <% checkWrite("map_vid_04"); %>;
	map_vid[5] = <% checkWrite("map_vid_10"); %>;
	map_vid[6] = <% checkWrite("map_vid_11"); %>;
	map_vid[7] = <% checkWrite("map_vid_12"); %>;
	map_vid[8] = <% checkWrite("map_vid_13"); %>;
	map_vid[9] = <% checkWrite("map_vid_14"); %>;
var backhaul_tmp;

//--------------------Multi AP----------------------//
/********************************************************************
**          on document load
********************************************************************/
function on_init()
{
	if(easymesh_enabled) {
		document.getElementsByName("wl_ssid1")[0].readOnly = true;
		document.getElementsByName("wl_hide_ssid1")[0].disabled = true;
	}
	Load_Setting();
}

function DisplayTxRate(v_index, band, auto, txrate, rf_num, channel_width)
{
	var mask, defidx, i, rate, vht_num;

	var band_index, band_now;
	band_index = document.MultipleAP.elements["wl_band"+v_index].selectedIndex;
	band_now = parseInt(document.MultipleAP.elements["wl_band"+v_index].options[band_index].value, 10)+1;
	if (band > band_now)
		band = band_now;

	mask=0;
	if (auto)
		txrate=0;
	if (band & 1)
		mask |= 1;
	if ((band&2) || (band&4))
		mask |= 2;
	if (band & 8) {
		if (rf_num == 4)
			mask |= 60;
		else if (rf_num == 3)
			mask |= 28;
		else if (rf_num == 2)
			mask |= 12;
		else
			mask |= 4;
	}
	if(band & 64)
		mask |= 64;
	if (band & 128)
		mask |= 128;
	defidx=0;
	document.write('<option value="' + 0 + '">' + rate_name[0] + '\n');
	for (idx=1, i=1; i<rate_name.length; i++) {
		if (rate_mask[i] & mask) {
			if(wlan_support_11ax == 1 && wlan_wifi5_wifi6_comp == 0){
				if(!auto){
					if(i<13 && (txrate == (i-1)))
						defidx = idx;
					else if(i>=13 && txrate == (0x80 + i-13))
						defidx = idx;
				}
			}
			else {
				if(txrate == 0)
					defidx = 0;
				else if(i<29 && (txrate == (1 << (i-1)))){
					defidx = idx;
				}else if (i>=29 && (txrate == ((1<<28)+(i-29)))){
					defidx = idx;
				}
				/*
				if (i == 0)
					rate = 0;
				else
					rate = (1 << (i-1));
				if (txrate == rate)
					defidx = idx;
				else if (i>=29 && (txrate == ((1<<28)+(i-29))))
					defidx = idx;
				*/
			}
			document.write('<option value="' + i + '">' + rate_name[i] + '\n');
			idx++;
		}
	}
	if(band & 64){
		vht_num = rf_num*10-1;
		for (i=0; i<=vht_num; i++) {
				if(wlan_support_11ax == 1 && wlan_wifi5_wifi6_comp == 0)
					rate = 0x100 + Math.floor(i/10)*(0x10)+(i%10);
				else
					rate = (((1 << 31)>>>0) + i);
				if (txrate == rate)
					defidx = idx;
				if(channel_width!=0 || (i!=9 && i!=19 && i!=29 && i!=39))//no MCS9 when 20M
				{
					document.write('<option value="' + (i+45) + '">' + vht_rate_name[i] + '\n');
					idx++;
				}
		}
	}
	if(band & 128){
		he_num = rf_num*12-1;
		for (i=0; i<=he_num; i++) {
				if(wlan_support_11ax == 1 && wlan_wifi5_wifi6_comp == 0)
					rate = 0x180 + Math.floor(i/12)*(0x10)+(i%12);
				else
					rate = (((1 << 31)>>>0) + 40 + i);
				if (txrate == rate)
					defidx = idx;
				document.write('<option value="' + (i+85) + '">' + he_rate_name[i] + '\n');
				idx++;
		}
	}

	document.MultipleAP.elements["TxRate"+v_index].selectedIndex=defidx;
}

function openWindow(url, windowName, wide, high) {
	if (document.all)
		var xMax = screen.width, yMax = screen.height;
	else if (document.layers)
		var xMax = window.outerWidth, yMax = window.outerHeight;
	else
		var xMax = 640, yMax=500;
	var xOffset = (xMax - wide)/2;
	var yOffset = (yMax - high)/3;

	var settings = 'width='+wide+',height='+high+',screenX='+xOffset+',screenY='+yOffset+',top='+yOffset+',left='+xOffset+', resizable=yes, toolbar=no,location=no,directories=no,status=no,menubar=no,scrollbars=yes';
	window.open(url, windowName, settings);
}

function open_client_table(id)
{
	aclist_index = id;
	openWindow('/admin/wlstatbl_vap.asp?id='+id, 'showWirelessClient', 700, 400);
}

function click_reset()
{
	location.reload(true);
/*
	for (i=1; i<=4; i++) {
		document.MultipleAP["wl_disable" + i].checked = (virtual_wlan_enable[i] ? true :  false);		
		UpdateVirtualStatus(document.MultipleAP, i);		
	}
*/
}

function saveChanges(form, wlan_id)
{
	var idx;
 	for (idx=1; idx<=4; idx++) {
        var wlan_state = 0;
			ssid = form.elements["wl_ssid"+idx];		
		if (form.elements["wl_disable"+idx].checked) {
			//ssid = form.elements["wl_ssid"+idx];
			if (ssid.value == "") {
				alert("<% multilang(LANG_SSID_CANNOT_BE_EMPTY); %>");
				ssid.value = ssid.defaultValue;
				ssid.focus();
				return false;
			}	   	
			if (byteLength(ssid.value)>32) {
				alert("<% multilang(LANG_SSID_LENGTH_SHOULD_BE_MAXIMUM_32_OCTETS); %>");
				ssid.focus();
				return false;
			}
			wlan_state = 1;
		}
		if(form.elements["wl_limitstanum"+idx].value == 1){
			if(!isNumber(form.elements["wl_stanum"+idx].value)){
				alert("<% multilang(LANG_INVALID_LIMIT_ASSOCIATED_CLIENT_NUMBER); %>");
				form.elements["wl_stanum"+idx].focus();
				return false;
			}
		}
        if (enable_vlan) {
			if (map_vid[wlan_id * 5 + idx]) {
				if ((map_fronthaul[wlan_id * 5 + idx] != wlan_state) || (ssid.value != map_ssid[wlan_id * 5 + idx])) {
					if (!confirm("Modification on SSID "+ssid.value+" will be automatically synchronized to VLAN settings. Click OK to continue.")) {
						return false;
					}
					if (!map_fronthaul[0]) {
						for (i = 1; i < 5; i++) {
							map_fronthaul[i] = 0;
						}
					}
					if (!map_fronthaul[5]) {
						for (i = 6; i < 10; i++) {
							map_fronthaul[i] = 0;
						}
					}
					for (i = 0; i < 10; i++) {
						if(!map_fronthaul[i])
							map_ssid[i] = "";
					}
					if(-1 != map_ssid.indexOf(ssid.value))
						map_vid[wlan_id * 5 + idx] = map_vid[map_ssid.indexOf(ssid.value)];
				}
			} else if ((idx != backhaul_tmp) && wlan_state) {
				if (!confirm("Created SSID "+ssid.value+" is about to be categorized under Default Guest Network or change in VLAN page. Click OK to continue.")) {
					return false;
				}
				map_vid[wlan_id * 5 + idx] = default_sec_vid;
			}
			document.getElementById("map_vid_info").value = map_vid.join('_');
		}

	}
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
}

function showtxrate_updated_virtual(form, wlan_id, original_wmm_enable, channel_width)
{
  	var idx=0;
  	var i;
  	var txrate_str;
	var band, band_value, current_value, txrate_idx;
	var encrypt;
	var rf_num= <% checkWrite("rf_used"); %> ;
	var mask;

	txrate_idx = form.elements["TxRate"+wlan_id].selectedIndex;
	current_value = form.elements["TxRate"+wlan_id].options[txrate_idx].value;
	i = form.elements["wl_band"+wlan_id].selectedIndex;
	band = form.elements["wl_band"+wlan_id].options[i].value;
	band_value = parseInt(band, 10)+1;
	encrypt=wlan_encrypt[wlan_id];
	if(encrypt==1){ //wep
		if(band>3){
			alert("<% multilang(LANG_ERROR_BAND_NOT_SPPORT_WEP); %>");
			window.location.reload(true);
			return;
		}
	}
	form.elements["TxRate"+wlan_id].length=0;

	mask=0;
	if (band_value & 1)
		mask |= 1;
	if ((band_value&2) || (band_value&4))
		mask |= 2;
	if (band_value & 8) {
		if (rf_num == 4)
			mask |= 60;
		else if (rf_num == 3)
			mask |= 28;
		else if (rf_num == 2)
			mask |= 12;
		else
			mask |= 4;
	}
	if(band_value & 64)
		mask |= 64;

	for (idx=0, i=0; i<rate_name.length; i++) {
		if (rate_mask[i] & mask) {
			form.elements["TxRate"+wlan_id].options[idx++] = new Option(rate_name[i], i, false, false);
		}
	}
	if(band_value & 64){
		vht_num = rf_num*10-1;
		for (i=0; i<=vht_num; i++) {
			if(channel_width!=0 || (i!=9 && i!=19 && i!=29 && i!=39))//no MCS9 when 20M
			{
				form.elements["TxRate"+wlan_id].options[idx++] = new Option(vht_rate_name[i], i+45, false, false);
			}
		}
	}

	if (band ==7 || band ==9 || band ==10 || band==11 || band==63|| band==71|| band==75 || ((parseInt(band, 10)+1) & 128)) {
		form.elements["wl_wmm_capable"+wlan_id].selectedIndex = 1;
		disableTextField(form.elements["wl_wmm_capable"+wlan_id]);
	}
	else {
		if (original_wmm_enable)
			form.elements["wl_wmm_capable"+wlan_id].selectedIndex = 1;
		else
			form.elements["wl_wmm_capable"+wlan_id].selectedIndex = 0;

		enableTextField(form.elements["wl_wmm_capable"+wlan_id]);
	}
	form.elements["TxRate"+wlan_id].length = idx;

	form.elements["TxRate"+wlan_id].selectedIndex = 0;
 	for (i=0; i<idx; i++) {
		txrate_str = form.elements["TxRate"+wlan_id].options[i].value;
		if (current_value == txrate_str){
			form.elements["TxRate"+wlan_id].selectedIndex = i;
			break;
		}
	}
}

function showBand_MultipleAP(form, wlan_id, band_root, index_id)
{
	var idx = 0;
	var band_value = virtual_bandIdx[index_id];
/*
	if (band_root == 11) {	//11:5G
		form.elements["wl_band"+index_id].options[idx++] = new Option("5 GHz (A+N)", "11", false, false);
	}
	else {
		form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (B)", "0", false, false);
		form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (G)", "1", false, false);
		form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (N)", "7", false, false);
		form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (B+G)", "2", false, false);
		form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (G+N)", "9", false, false);
		form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (B+G+N)", "10", false, false);
	}
*/	
	if (band_root ==0) {
		form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (B)", "0", false, false);
	}
	else if (band_root ==1 && wlan_support_deny_legacy ==1) {
		form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (G)", "1", false, false);
	}
	else if (band_root ==2) {
		form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (B)", "0", false, false);
		if (wlan_support_deny_legacy ==1)
			form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (G)", "1", false, false);
		form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (B+G)", "2", false, false);
	}
	else if (band_root ==9 && wlan_support_deny_legacy ==1) {
		form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (G)", "1", false, false);
		form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (N)", "7", false, false);
		form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (G+N)", "9", false, false);
	}
	else if (band_root ==10) {
		form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (B)", "0", false, false);
		if(wlan_support_deny_legacy ==1)
			form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (G)", "1", false, false);
		form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (B+G)", "2", false, false);
		if(wlan_support_deny_legacy ==1){
			form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (N)", "7", false, false);
			form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (G+N)", "9", false, false);
		}
		form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (B+G+N)", "10", false, false);
	}
	else if (band_root ==3) {
		form.elements["wl_band"+index_id].options[idx++] = new Option("5 GHz (A)", "3", false, false);
	}
	else if (band_root ==7 && wlan_support_deny_legacy ==1) {
		var Band2G5GSupport=<% checkWrite("Band2G5GSupport"); %>;
		if(Band2G5GSupport==1)//2g
			form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (N)", "7", false, false);
		else if(Band2G5GSupport==2)//5g
			form.elements["wl_band"+index_id].options[idx++] = new Option("5 GHz (N)", "7", false, false);
	}
	else if (band_root ==11) {
		form.elements["wl_band"+index_id].options[idx++] = new Option("5 GHz (A)", "3", false, false);
		if(wlan_support_deny_legacy ==1)
			form.elements["wl_band"+index_id].options[idx++] = new Option("5 GHz (N)", "7", false, false);
		form.elements["wl_band"+index_id].options[idx++] = new Option("5 GHz (A+N)", "11", false, false);
	}
	else if (band_root ==63 && wlan_support_deny_legacy ==1) {
		form.elements["wl_band"+index_id].options[idx++] = new Option("5 GHz (AC)", "63", false, false);
	}
	else if (band_root ==71 && wlan_support_deny_legacy ==1) {
		form.elements["wl_band"+index_id].options[idx++] = new Option("5 GHz (N)", "7", false, false);
		form.elements["wl_band"+index_id].options[idx++] = new Option("5 GHz (AC)", "63", false, false);
		form.elements["wl_band"+index_id].options[idx++] = new Option("5 GHz (N+AC)", "71", false, false);
	}
	else if (band_root ==75) {
		form.elements["wl_band"+index_id].options[idx++] = new Option("5 GHz (A)", "3", false, false);
		if(wlan_support_deny_legacy ==1)
			form.elements["wl_band"+index_id].options[idx++] = new Option("5 GHz (N)", "7", false, false);
		form.elements["wl_band"+index_id].options[idx++] = new Option("5 GHz (A+N)", "11", false, false);
		if(wlan_support_deny_legacy ==1){
			form.elements["wl_band"+index_id].options[idx++] = new Option("5 GHz (AC)", "63", false, false);
			form.elements["wl_band"+index_id].options[idx++] = new Option("5 GHz (N+AC)", "71", false, false);
		}
		form.elements["wl_band"+index_id].options[idx++] = new Option("5 GHz (A+N+AC)", "75", false, false);
	}
	else if(band_root ==127  && wlan_support_deny_legacy ==1){
		var Band2G5GSupport=<% checkWrite("Band2G5GSupport"); %>;
		if(Band2G5GSupport==1)//2g
			form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (AX)", "127", false, false);
		else if(Band2G5GSupport==2)//5g
			form.elements["wl_band"+index_id].options[idx++] = new Option("5 GHz (AX)", "127", false, false);
	}
	else if(band_root ==135  && wlan_support_deny_legacy ==1){
		form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (N)", "7", false, false);
		form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (AX)", "127", false, false);
		form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (N+AX)", "135", false, false);
	}
	else if(band_root ==137 && wlan_support_deny_legacy ==1){
		form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (G)", "1", false, false);
		form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (N)", "7", false, false);
		form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (G+N)", "9", false, false);
		form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (AX)", "127", false, false);
		form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (N+AX)", "135", false, false);
		form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (G+N+AX)", "137", false, false);
	}
	else if(band_root ==138){
		form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (B)", "0", false, false);
		if(wlan_support_deny_legacy ==1)
			form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (G)", "1", false, false);
		form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (B+G)", "2", false, false);
		if(wlan_support_deny_legacy ==1){
			form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (N)", "7", false, false);
			form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (G+N)", "9", false, false);
		}
		form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (B+G+N)", "10", false, false);
		if(wlan_support_deny_legacy ==1){
			form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (AX)", "127", false, false);
			form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (N+AX)", "135", false, false);
			form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (G+N+AX)", "137", false, false);
		}
		form.elements["wl_band"+index_id].options[idx++] = new Option("2.4 GHz (B+G+N+AX)", "138", false, false);
	}
	else if(band_root ==191 && wlan_support_deny_legacy ==1){
		form.elements["wl_band"+index_id].options[idx++] = new Option("5 GHz (AC)", "63", false, false);
		form.elements["wl_band"+index_id].options[idx++] = new Option("5 GHz (AX)", "127", false, false);
		form.elements["wl_band"+index_id].options[idx++] = new Option("5 GHz (AC+AX)", "191", false, false);
	}
	else if(band_root ==199 && wlan_support_deny_legacy ==1){
		form.elements["wl_band"+index_id].options[idx++] = new Option("5 GHz (N)", "7", false, false);
		form.elements["wl_band"+index_id].options[idx++] = new Option("5 GHz (AC)", "63", false, false);
		form.elements["wl_band"+index_id].options[idx++] = new Option("5 GHz (N+AC)", "71", false, false);
		form.elements["wl_band"+index_id].options[idx++] = new Option("5 GHz (AX)", "127", false, false);
		form.elements["wl_band"+index_id].options[idx++] = new Option("5 GHz (AC+AX)", "191", false, false);
		form.elements["wl_band"+index_id].options[idx++] = new Option("5 GHz (N+AC+AX)", "199", false, false);
	}
	else if(band_root ==203){
		form.elements["wl_band"+index_id].options[idx++] = new Option("5 GHz (A)", "3", false, false);
		if(wlan_support_deny_legacy ==1)
			form.elements["wl_band"+index_id].options[idx++] = new Option("5 GHz (N)", "7", false, false);
		form.elements["wl_band"+index_id].options[idx++] = new Option("5 GHz (A+N)", "11", false, false);
		if(wlan_support_deny_legacy ==1){
			form.elements["wl_band"+index_id].options[idx++] = new Option("5 GHz (AC)", "63", false, false);
			form.elements["wl_band"+index_id].options[idx++] = new Option("5 GHz (N+AC)", "71", false, false);
		}
		form.elements["wl_band"+index_id].options[idx++] = new Option("5 GHz (A+N+AC)", "75", false, false);
		if(wlan_support_deny_legacy ==1){
			form.elements["wl_band"+index_id].options[idx++] = new Option("5 GHz (AX)", "127", false, false);
			form.elements["wl_band"+index_id].options[idx++] = new Option("5 GHz (AC+AX)", "191", false, false);
			form.elements["wl_band"+index_id].options[idx++] = new Option("5 GHz (N+AC+AX)", "199", false, false);
		}
		form.elements["wl_band"+index_id].options[idx++] = new Option("5 GHz (A+N+AC+AX)", "203", false, false);
	}
	
	if (band_value > band_root)
		band_value = band_root;
	for (i=0 ; i<idx ; i++) {
		if (form.elements["wl_band"+index_id].options[i].value == band_value) {
			form.elements["wl_band"+index_id].selectedIndex = i;
			break;
		}
	}
}

var backhaulIndex0 = <% checkWrite("backhaulIndexQuery_0"); %>;
var backhaulIndex1 = <% checkWrite("backhaulIndexQuery_1"); %>;

function Load_Setting() {
	var map_role = <% checkWrite("multi_ap_controller"); %>;
	if(0 != map_role) {
		if (wlan_idx == 0) {
			//disableTextField(document.MultipleAP.elements["wl_ssid"+backhaulIndex0]);
			//disableTextField(document.MultipleAP.elements["wl_hide_ssid"+backhaulIndex0]);
			backhaul_tmp = backhaulIndex0;
		} else if (wlan_idx == 1) {
			//disableTextField(document.MultipleAP.elements["wl_ssid"+backhaulIndex1]);
			//disableTextField(document.MultipleAP.elements["wl_hide_ssid"+backhaulIndex1]);
			backhaul_tmp = backhaulIndex1;
		}
	}
}

function enableVirtualWLAN(form, virtual_id)
{
	enableTextField(form.elements["wl_band"+virtual_id]);
	enableTextField(form.elements["wl_ssid"+virtual_id]);
	enableTextField(form.elements["TxRate"+virtual_id]);
	if(isCMCCSupport==1)
	  enableTextField(form.elements["wl_pri_ssid"+virtual_id]);
	enableTextField(form.elements["wl_hide_ssid"+virtual_id]);
/*
	if (form.elements["wl_band"+virtual_id].selectedIndex == 2 || 
		form.elements["wl_band"+virtual_id].selectedIndex >= 4) {
			form.elements["wl_wmm_capable"+virtual_id].selectedIndex = 1;		
			disableTextField(form.elements["wl_wmm_capable"+virtual_id]);
	}
	else 
		enableTextField(form.elements["wl_wmm_capable"+virtual_id]);
*/
	enableTextField(form.elements["wl_access"+virtual_id]);		
	enableTextField(form.elements["aclient"+virtual_id]);

	var i;
	i = form.elements["wl_band"+virtual_id].selectedIndex;
	band = form.elements["wl_band"+virtual_id].options[i].value;
	if(band ==7 || band ==9 || band ==10 || band==11 || band==63|| band==71|| band==75 || ((parseInt(band, 10)+1) & 128))
	{
		form.elements["wl_wmm_capable"+virtual_id].selectedIndex = 1;
		disableTextField(form.elements["wl_wmm_capable"+virtual_id]);
	}
	else
		enableTextField(form.elements["wl_wmm_capable"+virtual_id]);
	
	enableTextField(form.elements["wl_limitstanum"+virtual_id]);
	check_stanum(document.MultipleAP, virtual_id);

	enableTextField(form.elements["wl_txbf_ssid"+virtual_id]);
	UpdateTxBf(form, virtual_id);
	enableTextField(form.elements["wl_mc2u_ssid"+virtual_id]);
}

function disableVirtualWLAN(form, virtual_id)
{
	disableTextField(form.elements["wl_band"+virtual_id]);
	disableTextField(form.elements["wl_ssid"+virtual_id]);
	disableTextField(form.elements["TxRate"+virtual_id]);
	if(isCMCCSupport==1)
	  disableTextField(form.elements["wl_pri_ssid"+virtual_id]);
	disableTextField(form.elements["wl_hide_ssid"+virtual_id]);
	disableTextField(form.elements["wl_wmm_capable"+virtual_id]);
	disableTextField(form.elements["wl_access"+virtual_id]);	
	disableTextField(form.elements["aclient"+virtual_id]);
	disableTextField(form.elements["wl_limitstanum"+virtual_id]);
	disableTextField(form.elements["wl_stanum"+virtual_id]);
	disableTextField(form.elements["wl_txbf_ssid"+virtual_id]);
	disableTextField(form.elements["wl_txbfmu_ssid"+virtual_id]);
	disableTextField(form.elements["wl_mc2u_ssid"+virtual_id]);
}

function UpdateVirtualStatus(form, virtual_id)
{
	if (!form.elements["wl_disable"+virtual_id].checked)
		disableVirtualWLAN(form, virtual_id);
	else
		enableVirtualWLAN(form, virtual_id);
}

function check_stanum(form, virtual_id)
{
	if (form.elements["wl_limitstanum"+virtual_id].value == 0)
		disableTextField(form.elements["wl_stanum"+virtual_id]);
	else
		enableTextField(form.elements["wl_stanum"+virtual_id]);
}

function UpdateTxBf(form, virtual_id)
{
	if (form.elements["wl_txbf_ssid"+virtual_id].value == 0)
		disableTextField(form.elements["wl_txbfmu_ssid"+virtual_id]);
	else
		enableTextField(form.elements["wl_txbfmu_ssid"+virtual_id]);
}

</script>
<style>
.intro_title {
    color: #a3238e;
    font-size: 16px;
    font-weight: bold;
}
.intro_content {
    color: #666;
    font-size: 12px;
}
a {
    color: #5e5e5e;
    text-decoration: none;
}
body {
    font: 9px Arial,toTahoma,Helvetica,sans-serif;
    background: #fff;
}
table,
th,
td,th {
 margin:0;
 padding: 2px 1px;
 font-weight: normal;
 border:1px solid #ccc;
 text-align:center;
}
table {
 border-collapse:collapse;
 border-spacing:0;
}
.table_data{font-size: 9px !important;}
.table_data table th, .table_data table td {
    font-size: 9px !important;
}
input[type="text"], input[type="password"], select {
    border: 1px solid #7e9db9;
    padding:  0 !important;
    margin-left: 0 !important;
    height: 22px;
    font-size: 9px !important;
    font-weight: normal;
}
input, select, textarea {
    -moz-box-sizing: border-box;
    -webkit-box-sizing: border-box;
    -ms-box-sizing: border-box;
    box-sizing: border-box;
    margin: 0 !important;
	font-size: 9px !important;
}
select {
 -webkit-appearance: none;
 font-size: 12px;
}
select option {
 font-size: 8px; 
}
</style>
</head>

<body onLoad="on_init();">
<div class="intro_main ">
	<p class="intro_title">Multiple APs</p>
	<p class="intro_content">This page shows and updates the wireless setting for multiple APs.</p>
</div>
<br>
<form action=/boaform/formWlanMultipleAP method=POST name="MultipleAP">
<input type="hidden" value="<% checkWrite("wlan_idx"); %>" name="wlanIdx">		
	 <% checkWrite("blockingvap"); %>
<!--
	<tr>
		<td><b><% multilang(LANG_BLOCKING_BETWEEN_VAP); %>:</b></td>
		<td><input type="radio" name="mbssid_block" value="disable" ><% multilang(LANG_DISABLE); %>&nbsp;&nbsp;
		<input type="radio" name="mbssid_block" value="enable" ><% multilang(LANG_ENABLE); %></td>
	</tr>
-->
	<div class="table_data">
		<table> 
			<tr style="background:#e9e9e9;">
			   <th>No.</th>
			   <th>Enable</th>
			   <th id="wlan_band" <% checkWrite("wlan_qtn_hidden_function"); %>>Band</th>
			   <th>SSID</th>
			   <th <% checkWrite("wlan_qtn_hidden_function"); %> width="60">Data Rate</th>
			   <script>
               if(isCMCCSupport==1)
   		       document.write('<th>SSID Priority</th>'); 
               </script>
			   <th>Broadcast SSID</th>
			   <th>WMM</th>
			   <th><% multilang(LANG_RELAY_BLOCKING); %></th>   
			   <th id="wl_limit_stanum_div" style="display:none"><% multilang(LANG_STA_NUMBER); %></th>
			   <th>Active Client List</th>
			   <th id="wl_txbf_div" style="display:none"><% multilang(LANG_TXBF); %></th>
			   <th id="wl_txbfmu_div" style="display:none"><% multilang(LANG_TXBF_MU); %></th>
			   <th id="wl_mc2u_div" style="display:none"><% multilang(LANG_MC2U); %></th>
			</tr>
			<tr>
				<td>AP1</td>
				<td>
				<script type="text/javascript">
					var wlanDisabled = <% getVirtualIndex("wlanDisabled", "1"); %>;
					if (wlanDisabled == "0") {
						if (easymesh_enabled) {
							document.write('<input type="checkbox" checked="checked" disabled>');
							document.write('<input type="hidden" name="wl_disable1" value="ON" onClick="UpdateVirtualStatus(document.MultipleAP, 1)" checked="checked">');
						} else {
							document.write('<input type="checkbox" name="wl_disable1" value="ON" onClick="UpdateVirtualStatus(document.MultipleAP, 1)" checked="checked">');
						}
					} else {
						if (easymesh_enabled) {
							document.write('<input type="checkbox" name="wl_disable1" value="ON" onClick="UpdateVirtualStatus(document.MultipleAP, 1)" disabled>');
						} else {
							document.write('<input type="checkbox" name="wl_disable1" value="ON" onClick="UpdateVirtualStatus(document.MultipleAP, 1)">');
						}
					}
			</script>
		</td>
		<td id="wlan_band1" <% checkWrite("wlan_qtn_hidden_function"); %>>
			<select style="width:110" name=wl_band1 onChange="showtxrate_updated_virtual(document.MultipleAP, 1, virtual_wmm_enable[1] , chanwid)">
				<script>
					showBand_MultipleAP(document.MultipleAP, wlan_idx, bandIdx[wlan_idx], 1);
	     			</script>
			</select>
		</td>
		<td>
			<script>
				if (easymesh_enabled) {
					document.write('<input type="text" size="7" maxlength="32" value="<% SSIDStr("vap0"); %>" disabled>');
					document.write('<input type="hidden" name="wl_ssid1" size="7" maxlength="32" value="<% SSIDStr("vap0"); %>">');
				}
				else {
					document.write('<input type="text" name="wl_ssid1" size="7" maxlength="32" value="<% SSIDStr("vap0"); %>">');
				}
			</script>
		</td>
		<td <% checkWrite("wlan_qtn_hidden_function"); %> width="60">
			<select name="TxRate1" width="60">
				<script>
		    			band = <% getVirtualIndex("band", "1"); %>;
		    			auto = <% getVirtualIndex("rateAdaptiveEnabled", "1"); %>;
	 	    			txrate = <% getVirtualIndex("fixTxRate", "1"); %>;
	 	    			rf_num = <% checkWrite("rf_used"); %>;		
					DisplayTxRate(1, band, auto, txrate, rf_num, chanwid);
				</script>
			</select>
		</td>
	<script>
	if(isCMCCSupport==1){
		document.write('<td>');
		document.write('<select name=wl_pri_ssid1>');
		document.write('<option value=\"0\">0</option>\n');
        document.write('<option value=\"1\">1</option>\n');
		document.write('<option value=\"2\">2</option>\n');
		document.write('<option value=\"3\">3</option>\n');
		document.write('<option value=\"4\">4</option>\n');
		ssidpri = <% getVirtualIndex("ssid_pri", "1"); %>;
		def_index = priToIndex(ssidpri); // 0,1,2,3,4
		document.MultipleAP.elements["wl_pri_ssid1"].selectedIndex=def_index;
		document.write('</select>');	
		document.write('</td>');	
	}
	</script>
		<td>
			<select name=wl_hide_ssid1>
				<option value="1">Disabled</option>
				<option value="0">Enabled</option>
				<script>
					if (virtual_hidessid[1])
						document.MultipleAP.elements["wl_hide_ssid1"].selectedIndex=0;
					else
						document.MultipleAP.elements["wl_hide_ssid1"].selectedIndex=1;
				</script>
			</select>
		</td>
		<td>
			<select name=wl_wmm_capable1>
				<option value="0">Disabled</option>
				<option value="1">Enabled</option>
				<script>
					if (virtual_wmm_enable[1])
						document.MultipleAP.elements["wl_wmm_capable1"].selectedIndex=1;
					else
						document.MultipleAP.elements["wl_wmm_capable1"].selectedIndex=0;
				</script>
			</select>
		</td>
		<td>
			<select name=wl_access1>
				<option value="0">Disabled</option>
				<option value="1">Enabled</option>
			</select>
		</td>
		<td id="wl_limit_stanum_div1" style="display:none">
			<select name=wl_limitstanum1 onChange="check_stanum(document.MultipleAP, 1)">
				<option value="0">Disabled</option>
				<option value="1">Enabled</option>
			</select>
			<input type="text" name="wl_stanum1" size="3" maxlength="2">
		</td>	
		<td>
			<input type="hidden" value="1" name="vap">	
			<input type="button" value="Show" name="aclient1" onClick="open_client_table(1);">
	    </td>
		<td id="wl_txbf_div1" style="display:none" >
			<select name=wl_txbf_ssid1 onChange="UpdateTxBf(document.MultipleAP, 1)">
				<option value="0">Disabled</option>
				<option value="1">Enabled</option>
			</select>
		</td>
		<td id="wl_txbfmu_div1" style="display:none" >
			<select name=wl_txbfmu_ssid1>
				<option value="0">Disabled</option>
				<option value="1">Enabled</option>
			</select>
		</td>
		<td id="wl_mc2u_div1" style="display:none" >
			<select name=wl_mc2u_ssid1>
				<option value="1">Disabled</option>
				<option value="0">Enabled</option>
			</select>
		</td>
	</tr>
	<script>
		if (mssid_num == 1) {
			document.write("</table>\n");
			document.write("<span id = \"hide_div\" class = \"off\">\n");
			document.write("<table border=\"1\" width=700>\n");
		}
	</script>
	<tr>
		<td>AP2</td>
		<td>
			<script type="text/javascript">
				var wlanDisabled = <% getVirtualIndex("wlanDisabled", "2"); %>;
				if (wlanDisabled == "0") {
					document.write('<input type="checkbox" name="wl_disable2" value="ON" onClick="UpdateVirtualStatus(document.MultipleAP, 2)" checked="checked">');
				}
				else {
					document.write('<input type="checkbox" name="wl_disable2" value="ON" onClick="UpdateVirtualStatus(document.MultipleAP, 2)">');
				}
			</script>
		</td>
		<td id="wlan_band2" <% checkWrite("wlan_qtn_hidden_function"); %> >
			<select style="width:110" name=wl_band2 onChange="showtxrate_updated_virtual(document.MultipleAP, 2, virtual_wmm_enable[2], chanwid)">
				<script>
					showBand_MultipleAP(document.MultipleAP, wlan_idx, bandIdx[wlan_idx], 2);
	     			</script>
			</select>
		</td>
		<td>
			<input type="text" name="wl_ssid2" size="7" maxlength="32" value="<% SSIDStr("vap1"); %>">
		</td>
		<td <% checkWrite("wlan_qtn_hidden_function"); %> width=60 >
			<select name="TxRate2">
				<script>
					band = <% getVirtualIndex("band", "2"); %>;
					auto = <% getVirtualIndex("rateAdaptiveEnabled", "2"); %>;
					txrate = <% getVirtualIndex("fixTxRate", "2"); %>;
					rf_num = <% checkWrite("rf_used"); %>;			

					DisplayTxRate(2, band, auto, txrate, rf_num, chanwid);
				</script>
			</select>
		</td>

	<script>
	if(isCMCCSupport==1){
		document.write('<td>');
		document.write('<select name=wl_pri_ssid2>');
		document.write('<option value=\"0\">0</option>\n');
        document.write('<option value=\"1\">1</option>\n');
		document.write('<option value=\"2\">2</option>\n');
		document.write('<option value=\"3\">3</option>\n');
		document.write('<option value=\"4\">4</option>\n');
		ssidpri = <% getVirtualIndex("ssid_pri", "2"); %>;
		def_index = priToIndex(ssidpri); // 0,1,2,3,4
		document.MultipleAP.elements["wl_pri_ssid2"].selectedIndex=def_index;
		document.write('</select>');	
		document.write('</td>');	
	}
	</script>
		
		<td>
			<select name=wl_hide_ssid2>
				<option value="1">Disabled</option>
				<option value="0">Enabled</option>
				<script>
					if (virtual_hidessid[2])
						document.MultipleAP.elements["wl_hide_ssid2"].selectedIndex=0;
					else
						document.MultipleAP.elements["wl_hide_ssid2"].selectedIndex=1;
				</script>
			</select>
		</td>
		<td>
			<select name=wl_wmm_capable2>
				<option value="0">Disabled</option>
				<option value="1">Enabled</option>
				<script>
					if (virtual_wmm_enable[2])
						document.MultipleAP.elements["wl_wmm_capable2"].selectedIndex=1;
					else
						document.MultipleAP.elements["wl_wmm_capable2"].selectedIndex=0;
				</script>
			</select>
		</td>
		<td >
			<select name=wl_access2>
				<option value="0">Disabled</option>
				<option value="1">Enabled</option>
			</select>
		</td>	
		<td id="wl_limit_stanum_div2" style="display:none">
			<select name=wl_limitstanum2 onChange="check_stanum(document.MultipleAP, 2)">
				<option value="0">Disabled</option>
				<option value="1">Enabled</option>
			</select>
			<input type="text" name="wl_stanum2" size="3" maxlength="2">
		</td>
		<td>
			<input type="button" value="Show" name="aclient2" onClick="open_client_table(2);">
	    </td>
		<td id="wl_txbf_div2" style="display:none" >
			<select name=wl_txbf_ssid2 onChange="UpdateTxBf(document.MultipleAP, 2)">
				<option value="0">Disabled</option>
				<option value="1">Enabled</option>
			</select>
		</td>
		<td id="wl_txbfmu_div2" style="display:none" >
			<select name=wl_txbfmu_ssid2>
				<option value="0">Disabled</option>
				<option value="1">Enabled</option>
			</select>
		</td>
		<td id="wl_mc2u_div2" style="display:none">
			<select name=wl_mc2u_ssid2>
				<option value="1">Disabled</option>
				<option value="0">Enabled</option>
			</select>
		</td>
	</tr>
	<tr>
		<td>AP3</td>
		<td>
			<script type="text/javascript">
				var wlanDisabled = <% getVirtualIndex("wlanDisabled", "3"); %>;
				if (wlanDisabled == "0") {
					document.write('<input type="checkbox" name="wl_disable3" value="ON" onClick="UpdateVirtualStatus(document.MultipleAP, 3)" checked="checked">');
				}
				else {
					document.write('<input type="checkbox" name="wl_disable3" value="ON" onClick="UpdateVirtualStatus(document.MultipleAP, 3)">');
				}
			</script>
		</td>
		<td id="wlan_band3" <% checkWrite("wlan_qtn_hidden_function"); %>>
			<select style="width:110" name=wl_band3 onChange="showtxrate_updated_virtual(document.MultipleAP, 3, virtual_wmm_enable[3], chanwid)">
				<script>
					showBand_MultipleAP(document.MultipleAP, wlan_idx, bandIdx[wlan_idx], 3);
	     			</script>
			</select>
		</td>
					<td>
			<input type="text" name="wl_ssid3" size="7" maxlength="32" value="<% SSIDStr("vap2"); %>">
		</td>
		<td <% checkWrite("wlan_qtn_hidden_function"); %> width=60>
			<select name="TxRate3">
				<script>
					band = <% getVirtualIndex("band", "3"); %>;
					auto = <% getVirtualIndex("rateAdaptiveEnabled", "3"); %>;
					txrate = <% getVirtualIndex("fixTxRate", "3"); %>;
					rf_num = <% checkWrite("rf_used"); %>;	

					DisplayTxRate(3, band, auto, txrate, rf_num, chanwid);
				</script>
			</select>
		</td>
	<script>
	if(isCMCCSupport==1){
		document.write('<td>');
		document.write('<select name=wl_pri_ssid3>');
		document.write('<option value=\"0\">0</option>\n');
        document.write('<option value=\"1\">1</option>\n');
		document.write('<option value=\"2\">2</option>\n');
		document.write('<option value=\"3\">3</option>\n');
		document.write('<option value=\"4\">4</option>\n');
		ssidpri = <% getVirtualIndex("ssid_pri", "3"); %>;
		def_index = priToIndex(ssidpri); // 0,1,2,3,4
		document.MultipleAP.elements["wl_pri_ssid3"].selectedIndex=def_index;
		document.write('</select>');	
		document.write('</td>');	
	}
	</script>
					<td>
			<select name=wl_hide_ssid3>
				<option value="1">Disabled</option>
				<option value="0">Enabled</option>
				<script>
					if (virtual_hidessid[3])
						document.MultipleAP.elements["wl_hide_ssid3"].selectedIndex=0;
					else
						document.MultipleAP.elements["wl_hide_ssid3"].selectedIndex=1;
				</script>
			</select>
		</td>
					<td>
			<select name=wl_wmm_capable3>
				<option value="0">Disabled</option>
				<option value="1">Enabled</option>
				<script>
					if (virtual_wmm_enable[3])
						document.MultipleAP.elements["wl_wmm_capable3"].selectedIndex=1;
					else
						document.MultipleAP.elements["wl_wmm_capable3"].selectedIndex=0;
				</script>
			</select>
		</td>
					<td>
			<select name=wl_access3>
				<option value="0">Disabled</option>
				<option value="1">Enabled</option>
			</select>
		</td>	
		<td id="wl_limit_stanum_div3" style="display:none">
			<select name=wl_limitstanum3 onChange="check_stanum(document.MultipleAP, 3)">
				<option value="0">Disabled</option>
				<option value="1">Enabled</option>
			</select>
			<input type="text" name="wl_stanum3" size="3" maxlength="2">
		</td>
		<td>
			<input type="button" value="Show" name="aclient3" onClick="open_client_table(3);">
	      </td>
		<td id="wl_txbf_div3" style="display:none">
			<select name=wl_txbf_ssid3 onChange="UpdateTxBf(document.MultipleAP, 3)">
				<option value="0">Disabled</option>
				<option value="1">Enabled</option>
			</select>
		</td>
		<td id="wl_txbfmu_div3" style="display:none">
			<select name=wl_txbfmu_ssid3>
				<option value="0">Disabled</option>
				<option value="1">Enabled</option>
			</select>
		</td>
		<td id="wl_mc2u_div3" style="display:none">
			<select name=wl_mc2u_ssid3>
				<option value="1">Disabled</option>
				<option value="0">Enabled</option>
			</select>
		</td>
	</tr>
	<script>
	  if (fon == 1 || mssid_num == 3) {
		document.write("</table>\n");
		document.write("<span id = \"hide_div\" class = \"off\">\n");
		document.write("<table border=\"1\" width=700>\n");
	  }
	</script>
	<tr>
		<td>AP4</td>
		<td>
			<script type="text/javascript">
				var wlanDisabled = <% getVirtualIndex("wlanDisabled", "4"); %>;
				if (wlanDisabled == "0") {
					document.write('<input type="checkbox" name="wl_disable4" value="ON" onClick="UpdateVirtualStatus(document.MultipleAP, 4)" checked="checked">');
				}
				else {
					document.write('<input type="checkbox" name="wl_disable4" value="ON" onClick="UpdateVirtualStatus(document.MultipleAP, 4)">');
				}
			</script>
		</td>
		<td id="wlan_band4" <% checkWrite("wlan_qtn_hidden_function"); %> >
			<select style="width:110" name=wl_band4 onChange="showtxrate_updated_virtual(document.MultipleAP, 4, virtual_wmm_enable[4], chanwid)">
				<script>
					showBand_MultipleAP(document.MultipleAP, wlan_idx, bandIdx[wlan_idx], 4);
	     			</script>
			</select>
		</td>
		<td>
			<input type="text" name="wl_ssid4" size="7" maxlength="32" value="<% SSIDStr("vap3"); %>">
		</td>
		<td <% checkWrite("wlan_qtn_hidden_function"); %> width = 60>
			<select name="TxRate4">
				<script>
					band = <% getVirtualIndex("band", "4"); %>;
					auto = <% getVirtualIndex("rateAdaptiveEnabled", "4"); %>;
					txrate = <% getVirtualIndex("fixTxRate", "4"); %>;
					rf_num = <% checkWrite("rf_used"); %>;	

					DisplayTxRate(4, band, auto, txrate, rf_num, chanwid);
				</script>
			</select>
		</td>
	<script>
	if(isCMCCSupport==1){
		document.write('<td>');
		document.write('<select name=wl_pri_ssid4>');
		document.write('<option value=\"0\">0</option>\n');
        document.write('<option value=\"1\">1</option>\n');
		document.write('<option value=\"2\">2</option>\n');
		document.write('<option value=\"3\">3</option>\n');
		document.write('<option value=\"4\">4</option>\n');
		ssidpri = <% getVirtualIndex("ssid_pri", "4"); %>;
		def_index = priToIndex(ssidpri); // 0,1,2,3,4
		document.MultipleAP.elements["wl_pri_ssid4"].selectedIndex=def_index;
		document.write('</select>');	
		document.write('</td>');	
	}
	</script>
		<td>
			<select name=wl_hide_ssid4>
				<option value="1">Disabled</option>
				<option value="0">Enabled</option>
				<script>
					if (virtual_hidessid[4])
						document.MultipleAP.elements["wl_hide_ssid4"].selectedIndex=0;
					else
						document.MultipleAP.elements["wl_hide_ssid4"].selectedIndex=1;
				</script>
			</select>
		</td>
		<td>
			<select name=wl_wmm_capable4>
				<option value="0">Disabled</option>
				<option value="1">Enabled</option>
				<script>
					if (virtual_wmm_enable[4])
						document.MultipleAP.elements["wl_wmm_capable4"].selectedIndex=1;
					else
						document.MultipleAP.elements["wl_wmm_capable4"].selectedIndex=0;
				</script>
			</select>
		</td>
					<td>
			<select name=wl_access4>
					<option value="0">Disabled</option>
					<option value="1">Enabled</option>
				</select>
			</td>	
			<td id="wl_limit_stanum_div4" style="display:none" >
				<select name=wl_limitstanum4 onChange="check_stanum(document.MultipleAP, 4)">
					<option value="0">Disabled</option>
					<option value="1">Enabled</option>
				</select>
				<input type="text" name="wl_stanum4" size="3" maxlength="2">
			</td>
			<td>
				<input type="button" value="Show" name="aclient4" onClick="open_client_table(4);">
		      </td>
			<td id="wl_txbf_div4" style="display:none">
				<select name=wl_txbf_ssid4 onChange="UpdateTxBf(document.MultipleAP, 4)">
					<option value="0">Disabled</option>
					<option value="1">Enabled</option>
				</select>
			</td>
			<td id="wl_txbfmu_div4" style="display:none">
				<select name=wl_txbfmu_ssid4>
					<option value="0">Disabled</option>
					<option value="1">Enabled</option>
				</select>
			</td>
			<td id="wl_mc2u_div4" style="display:none">
				<select name=wl_mc2u_ssid4>
					<option value="1">Disabled</option>
					<option value="0">Enabled</option>
				</select>
			</td>
		</tr>
		<script>
			<% initPage("wlmultipleap"); %>	
			for (i=1; i<=mssid_num; i++)
				UpdateVirtualStatus(document.MultipleAP, i);

			if (mssid_num == 1 || fon == 1) {
				document.write("</table>\n");
				document.write("</span>\n");
				document.write("<table border=\"1\" width=700>\n");
			}	
		</script>
	</table>
</div>
								
<div class="btn_ctl">
	<input type="hidden" value="/admin/wlmultipleap.asp" name="submit-url">
    <input type="hidden" value="" name="map_vid_info" id="map_vid_info">
	<input type="submit" value="Apply Changes"  onClick="return saveChanges(document.MultipleAP, wlan_idx)" class="link_bg">&nbsp;&nbsp;
	<input type="button" value="Reset" name="reset1" onClick="click_reset();" class="link_bg">&nbsp;&nbsp;  
	<!-- <input type="button" value=" Close " name="close" onClick="javascript: window.close();"> -->
	<input type="hidden" name="postSecurityFlag" value="">
</div>
<!--
<script>
document.MultipleAP.wl_ssid1.value = "<% SSIDStr("vap0"); %>";
document.MultipleAP.wl_ssid2.value = "<% SSIDStr("vap1"); %>";
document.MultipleAP.wl_ssid3.value = "<% SSIDStr("vap2"); %>";
document.MultipleAP.wl_ssid4.value = "<% SSIDStr("vap3"); %>";
</script>
-->
</form>
<br><br>
</body>
</html>
