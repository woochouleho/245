<% SendWebHeadStr();%>
<title>MAP-E <% multilang(LANG_MAPE_FMR_SETTING); %></title>
<SCRIPT>

function mapeFmrPostEntry(v6Prefix, v6PrefixLen, v4Prefix, v4PrefixLen, eaLen, offset)
{
	document.forms[0].mape_fmr_v6Prefix.value = v6Prefix;
	document.forms[0].mape_fmr_v6PrefixLen.value = v6PrefixLen;
	document.forms[0].mape_fmr_v4Prefix.value = v4Prefix;
	document.forms[0].mape_fmr_v4MaskLen.value = v4PrefixLen;
	document.forms[0].mape_fmr_eaLen.value = eaLen;
	document.forms[0].mape_fmr_psidOffset.value = offset;
}

function isRadioChecked(radioName){
	var len = document.forms[0].elements.length;
	var i = 0;
	for (i=0; i <len; i++){
		if (document.forms[0].elements[i].name == radioName){
			if (document.forms[0].elements[i].checked)
				return true;
		}
	}

	return false;
}

function mapeAddFmrClick(obj)
{
	if(document.forms[0].mape_fmr_v6Prefix.value == ""){
		alert('Please input the IPv6 prefix for fmr!');
		return false;
	}

	if(document.forms[0].mape_fmr_v6PrefixLen.value == ""){
		alert('Please input the IPv6 prefix length for fmr!');
		return false;
	}

	if(document.forms[0].mape_fmr_v4Prefix.value == ""){
		alert('Please input the IPv4 prefix for fmr!');
		return false;
	}

	if(document.forms[0].mape_fmr_v4MaskLen.value == ""){
		alert('Please input the IPv4 prefix length for fmr!');
		return false;
	}

	if(document.forms[0].mape_fmr_eaLen.value == ""){
		alert('Please input the EA length for fmr!');
		return false;
	}

	if(document.forms[0].mape_fmr_psidOffset.value == ""){
		alert('Please input the psid offset for fmr!');
		return false;
	}
	obj.isclick = 1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);

	return true;
}

function mapeModifyFmrClick(obj)
{
	var fmrEntrySelect = isRadioChecked("fmrSelect");	
	if (fmrEntrySelect == false) {		
		alert('Please select an FMR entry to modify!');
		return false;
	}
	obj.isclick = 1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
		
	return true;
}

function mapeRemoveFmrClick(obj)
{
	var fmrEntrySelect = isRadioChecked("fmrSelect");	
	if (fmrEntrySelect == false) {		
		alert('Please select an FMR entry to delect!');
		return false;
	}
	obj.isclick = 1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
		
	return true;
}

</SCRIPT>
</head>

<body>
<div class="intro_main ">
	<p class="intro_title">MAP-E <% multilang(LANG_MAPE_FMR_SETTING); %></p>
	<p class="intro_content"><% multilang(LANG_MAPE_FMR_SETTING_CONFIGURE_DESCRIPTION); %></p>
</div>

<form action=/boaform/formMAPE_FMR_Edit method=POST name="mape_fmr">

<%checkWrite("show_mape_fmrs");%>

<div class="btn_ctl">
<input type="hidden"  name="submit-url" value="/admin/mape_fmr_lists.asp">
<input type="hidden" name="postSecurityFlag" value="">
</div>

</form>

</body>
