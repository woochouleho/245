<% SendWebHeadStr();%>
<title>IPsec VPN Configuration</title>
<script language="javascript" src="common.js"></script>
<script type="text/javascript" src="base64_code.js"></script>
<script>
var conns = new Array();
with(conns){<% ipsecSwanConfDetail(); %>}

function checkParameters()
{
	with (getElById('ipsec'))
	{
		if(IKEAuthenticationMethod.value == 1){
			if(IKEPreshareKey.value == ""){
				alert("IKEPreshareKey cannot be empty in PSK mode");
				IKEPreshareKey.focus();
				return false;
			}
			else if((IKEPreshareKey.value.length < 8) || (IKEPreshareKey.value.length  >128)){
				alert("IKEPreshareKey character length is8~128");
				IKEPreshareKey.focus();
				return false;
			}
		}

		if(!checkString(IKEPreshareKey.value))
		{
			alert("<% multilang(LANG_INVALID_VALUE_IN_PASSWORD); %>");
			IKEPreshareKey.focus();
			return false;
		}
		encodepskValue.value = encode64(IKEPreshareKey.value);
		IKEPreshareKey.disabled=true;

		//check IKESAPeriod
		if((IKESAPeriod.value < 1200) || (IKESAPeriod.value > 86400)){
			alert("The IKESAPeriod value is illegal, the effective value is 1200~86400s");
			IKESAPeriod.focus();
			return false;
		}

		//check IPSecSATimePeriod
		if((IPSecSATimePeriod.value < 600) || (IPSecSATimePeriod.value > 86400)){
			alert("The IPSecSATimePeriod value is illegal, the effective value is 600~86400s");
			IPSecSATimePeriod.focus();
			return false;
		}

		//check IPSecSATrafficPeriod
		if((IPSecSATrafficPeriod.value < 2560) || (IPSecSATrafficPeriod.value > 536870912)){
			alert("The IPSecSATrafficPeriod value is illegal, the effective value is 2560~536870912KBytes");
			IPSecSATrafficPeriod.focus();
			return false;
		}

		if(DPDEnable.checked == true){
			//check DPDThreshold
			if((DPDThreshold.value < 10) || (DPDThreshold.value > 3600)){
				alert("The DPDThreshold value is illegal, the effective value is 10~3600s");
				DPDThreshold.focus();
				return false;
			}

			//check DPDRetry
			if((DPDRetry.value < 2) || (DPDRetry.value > 10)){
				alert("The DPD Retry value is illegal, the effective value is 2~10s");
				DPDRetry.focus();
				return false;
			}
		}
	}

	return true;
}

function btnSave()
{
	var	vForm = document.ipsec;

	if(checkParameters() == false){
		return;
	}

	vForm.submit();
}

function btnDelete()
{
	var	vForm = document.ipsec;
	vForm.OperatorStyle.value = "delete";
	vForm.submit();
}

function IPSecTypeChange()
{
	var type = getElById('IPSecType');
	var secRemoteIP = getElById('secRemoteIP');
	var secRemoteDomain = getElById('secRemoteDomain');

	if(type.value == 1){
		secRemoteIP.style.display="";
		secRemoteDomain.style.display="none";
	}
	else if(type.value == 2){
		secRemoteIP.style.display="none";
		secRemoteDomain.style.display="";
	}

	return;
}

function IKEAuthMethodChange()
{
	var method = getElById('IKEAuthenticationMethod');
	var secPsk = getElById('secPsk');

	if(method.value == 1){
		secPsk.style.display="";
	}
	else if(method.value == 2){
		secPsk.style.display="none";
	}

	return;
}

function IKEIDTypeChange()
{
	var type = getElById('IKEIDType');
	var secTypeName = getElById('secTypeName');

	if(type.value == 1){
		secTypeName.style.display="none";
	}
	else if(type.value == 2){
		secTypeName.style.display="";
	}

	return;
}

function IPSecTransformChange()
{
	var transform = getElById('IPSecTransform');
	var secEsp = getElById('secEsp');
	var secEsp2 = getElById('secEsp2');
	var secAh = getElById('secAh');

	if(transform.value == 1){
		secAh.style.display="";
		secEsp.style.display="none";
		secEsp2.style.display="none";
	}
	else if(transform.value == 2){
		secAh.style.display="none";
		secEsp.style.display="";
		secEsp2.style.display="";
	}

	return;
}

function DPDEnableChange()
{
	var DPDEnable = getElById('DPDEnable');
	var secDpd = getElById('secDpd');

	if(DPDEnable.checked == true){
		secDpd.style.display="";
	}
	else if(DPDEnable.checked == false){
		secDpd.style.display="none";
	}

	return;
}

function secChange()
{
	IPSecTypeChange();
	IKEAuthMethodChange();
	IKEIDTypeChange();
	IPSecTransformChange();
	DPDEnableChange();

	return;
}

function entry_init()
{
	var vForm = document.ipsec;

	vForm.OperatorStyle.value = "add";

	vForm.Enable.checked = true;
	vForm.encapl2tp.checked = true;
	vForm.IPSecType.value = 1;
	vForm.RemoteSubnet.value = "";
	vForm.LocalSubnet.value = "192.168.1.0/24";
	vForm.RemoteIP.value = "";
	vForm.RemoteDomain.value = "";
	//vForm.IPSecOutInterface.value
	vForm.IPSecEncapsulationMode.value = 1;
	vForm.IKEVersion.value = 2;
	vForm.ExchangeMode.value = 1;
	vForm.IKEAuthenticationAlgorithm.value = 1;
	vForm.IKEAuthenticationMethod.value = 1;
	vForm.IKEEncryptionAlgorithm.value = 1;
	vForm.IKEDHGroup.value = 1;
	vForm.IKEIDType.value = 1;
	vForm.IKELocalName.value = "";
	vForm.IKERemoteName.value = "";
	vForm.IKEPreshareKey.value = "";
	vForm.IKESAPeriod.value = "10800";
	vForm.IPSecTransform.value = 2;
	vForm.ESPAuthenticationAlgorithm.value = 1;
	vForm.ESPEncryptionAlgorithm.value = 2;
	vForm.AHAuthenticationAlgorithm.value = 1;
	vForm.IPSecPFS.value = 1;
	vForm.IPSecSATimePeriod.value = 3600;
	vForm.IPSecSATrafficPeriod.value = 1843200;
	vForm.DPDEnable.checked = false;
	vForm.DPDThreshold.value = 10;
	vForm.DPDRetry.value = 5;
	vForm.ConnectionStatus.value = 0;

	secChange();

	return;
}

function entry_change()
{
	var vForm = document.ipsec;
	var index = vForm.entrys.value;
	if(index == 999){
		entry_init();
		return;
	}

	vForm.OperatorStyle.value = "modify";

	var ipsecCfg = conns[index];

	if(ipsecCfg.Enable == 1){
		vForm.Enable.checked = true;
	}
	else{
		vForm.Enable.checked = false;
	}
	if(ipsecCfg.encapl2tp == 1){
		vForm.encapl2tp.checked = true;
	}
	else{
		vForm.encapl2tp.checked = false;
	}
	vForm.IPSecType.value = ipsecCfg.IPSecType;
	vForm.RemoteSubnet.value = ipsecCfg.RemoteSubnet;
	vForm.LocalSubnet.value = ipsecCfg.LocalSubnet;
	vForm.RemoteIP.value = ipsecCfg.RemoteIP;
	vForm.RemoteDomain.value = ipsecCfg.RemoteDomain;
	vForm.IPSecOutInterface.value = ipsecCfg.IPSecOutInterface;
	vForm.IPSecEncapsulationMode.value = ipsecCfg.IPSecEncapsulationMode;
	vForm.IKEVersion.value = ipsecCfg.IKEVersion;
	vForm.ExchangeMode.value = ipsecCfg.ExchangeMode;
	vForm.IKEAuthenticationAlgorithm.value = ipsecCfg.IKEAuthenticationAlgorithm;
	vForm.IKEAuthenticationMethod.value = ipsecCfg.IKEAuthenticationMethod;
	vForm.IKEEncryptionAlgorithm.value = ipsecCfg.IKEEncryptionAlgorithm;
	vForm.IKEDHGroup.value = ipsecCfg.IKEDHGroup;
	vForm.IKEIDType.value = ipsecCfg.IKEIDType;
	vForm.IKELocalName.value = ipsecCfg.IKELocalName;
	vForm.IKERemoteName.value = ipsecCfg.IKERemoteName;
	vForm.IKEPreshareKey.value = ipsecCfg.IKEPreshareKey;
	vForm.IKESAPeriod.value = ipsecCfg.IKESAPeriod;
	vForm.IPSecTransform.value = ipsecCfg.IPSecTransform;
	vForm.ESPAuthenticationAlgorithm.value = ipsecCfg.ESPAuthenticationAlgorithm;
	vForm.ESPEncryptionAlgorithm.value = ipsecCfg.ESPEncryptionAlgorithm;
	vForm.AHAuthenticationAlgorithm.value = ipsecCfg.AHAuthenticationAlgorithm;
	vForm.IPSecPFS.value = ipsecCfg.IPSecPFS;
	vForm.IPSecSATimePeriod.value = ipsecCfg.IPSecSATimePeriod;
	vForm.IPSecSATrafficPeriod.value = ipsecCfg.IPSecSATrafficPeriod;

	if(ipsecCfg.DPDEnable == 1){
		vForm.DPDEnable.checked = true;
	}
	else{
		vForm.DPDEnable.checked = false;
	}
	vForm.DPDThreshold.value = ipsecCfg.DPDThreshold;
	vForm.DPDRetry.value = ipsecCfg.DPDRetry;
	vForm.ConnectionStatus.value = ipsecCfg.ConnectionStatus;

	secChange();

	return;
}
function WebInit()
{
	var	vForm = document.ipsec;	
	if(conns.length > 0){
		vForm.entrys.value=0;
		entry_change();
	}
	secChange();
}

function show_password(id)
{
	var x= document.ipsec.IKEPreshareKey;

	if (x.type == "password") {
		x.type = "text";
	} else {
		x.type = "password";
	}
}
</script>
</head>
<body onload="WebInit();">
<div class="intro_main ">
	<p class="intro_title">IPsec VPN Configuration</p>
	<p class="intro_content"> This page is used to configure the parameters for IPsec mode VPN.</p>
</div>

<form action=/boaform/formIPsecStrongswan method=POST name="ipsec">
<div class="data_common data_common_notitle">
<div class="data_common">
	<input type="hidden" name="OperatorStyle" value="add">
	<table>
		<tr>
			<th>VPN Config</th>
			<td>
			<select name="entrys" class="selectStyle" onchange="entry_change()">
				<% ipsecSwanConfList();%>
			</select>
			</td>
		</tr>
		<tr>
			<th><% multilang(LANG_ENABLE); %>:</th>
			<td>
				<input type='checkbox' name='Enable' checked>
			</td>
		</tr>
		<tr>
			<th>L2TP Encap:</th>
			<td>
				<input type='checkbox' name='encapl2tp' checked>
			</td>
		</tr>
		<tr>
			<th>IPSecType:</th>
			<td>
			<select name="IPSecType" class="selectStyle" onchange="IPSecTypeChange();">
				<option value="1" selected>Site-to-Site</option>
				<option value="2">PC-to-Site</option>
			</select>
			</td>
		</tr>
		<tr>
			<th>RemoteSubnet:</th>
			<td>
				<input  name="RemoteSubnet" class="inputStyle" placeholder="Multiple network segments are separated by commas" type="text" />
			</td>
		</tr>
		<tr>
			<th>LocalSubnet:</th>
			<td>
				<input  name="LocalSubnet" class="inputStyle" value="192.168.1.0/24" type="text" />
			</td>
		</tr>

		<tr id="secRemoteIP" style="display: block">
			<th>RemoteIP:</th>
			<td>
				<input  name="RemoteIP" type="text" />
			</td>
		</tr>

		<tr id="secRemoteDomain" style="display: none">
			<th>RemoteDomain:</th>
			<td>
				<input  name="RemoteDomain" type="text" />
			</td>
		</tr>

		<tr>
			<th>IPSecOutInterface:</th>
			<td>
			<select name="IPSecOutInterface" class="selectStyle">
				<% if_wan_list("rt-any"); %>
			</select>
			</td>
		</tr>
		<tr>
			<th>IPSecEncapsulationMode:</th>
			<td>
			<select name="IPSecEncapsulationMode" class="selectStyle">
				<option value="1" selected>Tunnel</option>
				<option value="2">Transport</option>
			</select>
			</td>
		</tr>
		<tr>
			<td colspan="3"></td>
		</tr>
		<!-- -->
		<tr>
			<th>IKEVersion:</th>
			<td>
			<select name="IKEVersion" class="selectStyle">
				<option value="0">ALL</option>
				<option value="1">1</option>
				<option value="2" selected>2</option>
			</select>
			</td>
		</tr>
		<tr>
			<th>ExchangeMode:</th>
			<td>
			<select name="ExchangeMode" class="selectStyle">
				<option value="1" selected>main</option>
				<option value="2">aggressive</option>
			</select>
			</td>
		</tr>
		<tr>
			<th>IKEAuthenticationAlgorithm:</th>
			<td>
			<select name="IKEAuthenticationAlgorithm" class="selectStyle">
				<option value="1" selected>MD5</option>
				<option value="2">SHA1</option>
			</select>
			</td>
		</tr>
		<tr>
			<th>IKEAuthenticationMethod:</th>
			<td>
			<select name="IKEAuthenticationMethod" id="IKEAuthenticationMethod" class="selectStyle" onchange="IKEAuthMethodChange();">
				<option value="1" selected>PreShareKey</option>
				<option value="2">RsaSignature</option>
			</select>
			</td>
		</tr>
		<tr id = "secPsk" style="display: block">
			<th>IKEPreshareKey:</th>
			<td>
				<input name="IKEPreshareKey" class="inputStyle" type="password" onblur="checkit(this)" onfocus="clearit(this)" />
				<input type="checkbox" onClick="show_password()">Show Password
			</td>
		</tr>
		<tr>
			<th>IKEEncryptionAlgorithm:</th>
			<td>
			<select name="IKEEncryptionAlgorithm" class="selectStyle">
				<option value="1" selected>DES</option>
				<option value="2">3DES</option>
				<option value="3">AES128</option>
				<option value="4">AES192</option>
				<option value="5">AES256</option>
			</select>
			</td>
		</tr>
		<tr>
			<th>IKEDHGroup:</th>
			<td>
			<select name="IKEDHGroup" class="selectStyle">
				<option value="1" selected>Group1</option>
				<option value="2">Group2</option>
				<option value="3">Group5</option>
				<option value="4">Group14</option>
			</select>
			</td>
		</tr>
		<tr>
			<th>IKEIDType:</th>
			<td>
			<select name="IKEIDType" id="IKEIDType" class="selectStyle" onchange="IKEIDTypeChange();">
				<option value="1" selected>IP</option>
				<option value="2">Name</option>
			</select>
			</td>
		</tr>
		<tr id="secTypeName" style="display: none">
			<th>IKELocalName:</th>
			<td>
				<input name="IKELocalName" class="inputStyle" type="text" onblur="checkit(this)" onfocus="clearit(this)" />
			</td>
		</tr>
		<tr>
			<th>IKERemoteName:</th>
			<td>
				<input name="IKERemoteName" class="inputStyle" type="text" onblur="checkit(this)" onfocus="clearit(this)" />
			</td>
		</tr>
		<tr>
			<th>IKESAPeriod:</th>
			<td>
				<input  name="IKESAPeriod"class="inputStyle" placeholder="1200-86400s" type="text" value="10800"/>
			</td>
		</tr>
		<tr>
		</tr>
		<tr>
			<td colspan="3"></td>
		</tr>
		<!-- -->
		<tr>
			<th>IPSecTransform:</th>
			<td>
			<select name="IPSecTransform" id = "IPSecTransform" class="selectStyle" onchange="IPSecTransformChange();">
				<option value="1">AH</option>
				<option value="2" selected>ESP</option>
			</select>
			</td>
		</tr>
		<tr id="secEsp" style="display: block">
			<th>ESPAuthenticationAlgorithm:</th>
			<td>
			<select name="ESPAuthenticationAlgorithm" class="selectStyle">
				<option value="0">NONE</option>
				<option value="1" selected>MD5</option>
				<option value="2">SHA1</option>
			</select>
			</td>
		</tr>
		<tr id="secEsp2" style="display: block">
			<th>ESPEncryptionAlgorithm:</th>
			<td>
			<select name="ESPEncryptionAlgorithm" class="selectStyle">
				<option value="1">DES</option>
				<option value="2" selected>3DES</option>
				<option value="3">AES128</option>
				<option value="4">AES192</option>
				<option value="5">AES256</option>
			</select>
			</td>
		</tr>
		<tr id="secAh" style="display: none">
			<th>AHAuthenticationAlgorithm:</th>
			<td>
			<select name="AHAuthenticationAlgorithm" class="selectStyle">
				<option value="1">MD5</option>
				<option value="2">SHA1</option>
			</select>
			</td>
		</tr>
		<tr>
			<th>IPSecPFS:</th>
			<td>
			<select name="IPSecPFS" class="selectStyle">
				<option value="0" selected>NONE</option>
				<option value="1">Group1</option>
				<option value="2">Group2</option>
				<option value="3">Group5</option>
				<option value="4">Group14</option>
			</select>
			</td>
		</tr>
		<tr>
			<th>IPSecSATimePeriod:</th>
			<td>
				<input  name="IPSecSATimePeriod"class="inputStyle" placeholder="600-86400s" type="text" value="3600"/>
			</td>
		</tr>
		<tr>
			<th>IPSecSATrafficPeriod:</th>
			<td>
				<input  name="IPSecSATrafficPeriod" class="inputStyle" placeholder="2560-536870912Kbytes" type="text" value="1843200"/>
			</td>
		</tr>
		<tr>
			<td colspan="3"></td>
		</tr>
		<!-- -->
		<tr>
			<th>IPSec DPD (IPSec Dead Peer Detection on-demand):</th>
			<td>
				<input type='checkbox' name="DPDEnable" id="DPDEnable" value="ON" onchange="DPDEnableChange();">
			</td>
		</tr>
		<tr id="secDpd" style="display: none">
			<th>DPDThreshold:</th>
			<td>
				<input  name="DPDThreshold" class="inputStyle" placeholder="10-3600S" type="text" value="10"/>
			</td>
		</tr>
		<tr>
			<th>DPDRetry:</th>
			<td>
				<input  name="DPDRetry" class="inputStyle" placeholder="2-10S" type="text" value="5"/>
			</td>
		</tr>
		<tr>
			<td colspan="3"></td>
		</tr>
		<!-- -->
		<tr>
			<th>ConnectionStatus:</th>
			<td>
			<select name="ConnectionStatus" class="selectStyle" disabled>
				<option value="0" selected>Unconfigured</option>
				<option value="1">PhaseI_Connecting</option>
				<option value="2">PhaseII_Connecting</option>
				<option value="3">Connected</option>
				<option value="4">Disconnecting</option>
				<option value="5">Disconnected</option>
			</select>
			</td>
		</tr>
	</table>
</div>
</div>

<div class="btn_ctl">
	<input id=btnDel type="button" onclick="return btnDelete();" value="Delete">
	<input id=btnOK type="button" onclick="return btnSave();" value="Save">
	<input type="hidden" name="encodepskValue" value="">
	<input name="submit-url" type="hidden" value="/ipsec_swan.asp" />
</div>
</form>

<form action=/boaform/formIPSecSwanGenCert method=POST name="gen_local_cert">
<div class="column">
	<div class="column_title">
		<div class="column_title_left"></div>
			<p>Certificate Management</p>
		<div class="column_title_right"></div>
	</div>
	<div class="data_common">
	<table id="autoHead" class="tableStyle">
		<tr>
			<th>ca.cert.pem:</th>
			<td>
				<form action=/boaform/formIPSecSwanCert method=POST enctype="multipart/form-data" name="ipsec_cert">
					<input type="file" value="Choose File" name="binary" size=24>&nbsp;&nbsp;
					<input type="submit" value="Upload" name="load">
				</form>
			</td>
		</tr>
		<tr>
			<th>ca.pem(private key):</th>
			<td>
				<form action=/boaform/formIPSecSwanKey method=POST enctype="multipart/form-data" name="ipsec_key">
					<input type="file" value="Choose File" name="binary" size=24>&nbsp;&nbsp;
					<input type="submit" value="Upload" name="load">
					<input type="hidden" name="postSecurityFlag" value="">
				</form>
			</td>
		</tr>
		<tr>
			<th>Country(C):</th>
			<td>
				<input name="Country" class="inputStyle" type="text"/>
			</td>
		</tr>
		<tr>
			<th>Organization(O):</th>
			<td>
				<input name="Organization" class="inputStyle" type="text"/>
			</td>
		</tr>
		<tr>
			<th>generate a new local cert:</th>
			<td>
					<input type="submit" value="generate local cert" name="generate">
					<input name="submit-url" type="hidden" value="/ipsec_swan.asp" />
			</td>
		</tr>
	</table>
	</div>
</div>
</form>

</body>
</html>
