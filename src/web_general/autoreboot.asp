<% SendWebHeadStr();%>
<title>자동 Reboot <% multilang(LANG_CONFIGURATION); %></title>


<SCRIPT>

<!-- hide

var nDay;
var varLdapConfigServer					= ('<%getInfo("ldapConfigServer"); %>' == '1' ? true:false);
var	varEnblAutoReboot					= ('<%getInfo("enblAutoReboot"); %>' == '1'? true:false);
var	varEnblAutoRebootStaticConfig		= ('<%getInfo("enblAutoRebootStaticConfig"); %>' == '1'? true:false);
var	varAutoRebootOnIdle					= ('<%getInfo("autoRebootOnIdelServer"); %>' == '1' ? true:false);

function manualClicked()
{
	if (document.autoreboot.adslConnectionMode.value == 3) {
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

function naptClicked()
{
    if (document.autoreboot.cbEnblAutoRebootStaticConfig.value == 3) {
        // Route1483
        if (document.ethwan.cbEnblAutoRebootStaticConfig.checked == true) {
            document.ethwan.ipUnnum.checked = false;
            document.ethwan.ipUnnum.disabled = true;
        }
        else
            document.ethwan.ipUnnum.disabled = false;
        ipModeSelection();
    }
}


function setEnblAutoReboot() {
	with ( document.forms[0] ) {
		if(cbEnblAutoReboot.checked == true)
		{
			cbEnblAutoRebootStaticConfig.disabled = 0;
			setEnblAutoRebootStatic(1);
		}
		else
		{
			cbEnblAutoRebootStaticConfig.disabled = 1;
			setEnblAutoRebootStatic(0);
		}
	}
}


function setEnblAutoRebootStatic(bEnable) {
	document.autoreboot.cbEnblAutoRebootStaticConfig.checked = true;

	with ( document.forms[0] ) {
	var bDisabled;
		if(bEnable == 1)
		{
			if(cbEnblAutoRebootStaticConfig.checked == true)
				bDisabled = 0;
			else 
				bDisabled = 1;
		}
		else
			bDisabled = 1;

		txAutoRebootUpTime.disabled = bDisabled;
		cbAutoWanPortIdle.disabled = bDisabled;
		arSetStartTimeHour.disabled = bDisabled;
		arSetStartTimeMin.disabled = bDisabled;
		arSetEndTimeHour.disabled = bDisabled;
		arSetEndTimeMin.disabled = bDisabled;
		AutoRebootDisable(bDisabled);
		txAutoRebootAvrData.disabled = bDisabled;
		txAutoRebootAvrDataEPG.disabled = bDisabled;
	}

}

function AutoRebootDisable(nValue)
{
	with ( document.forms[0] ) {
		cbAutoRebootDay[0].disabled = nValue;
		cbAutoRebootDay[1].disabled = nValue;
		cbAutoRebootDay[2].disabled = nValue;
		cbAutoRebootDay[3].disabled = nValue;
		cbAutoRebootDay[4].disabled = nValue;
		cbAutoRebootDay[5].disabled = nValue;
		cbAutoRebootDay[6].disabled = nValue;
	}
}


function frmLoad() {
	with ( document.forms[0] ) {
		cbEnblAutoReboot.checked                = ('<%ejGet(enblAutoReboot)%>' == 1?true:false);
		cbEnblAutoRebootStaticConfig.checked	= ('<%ejGet(enblAutoRebootStaticConfig)%>' == 1?true:false);
		txAutoRebootUpTime.value				= '<%ejGet(autoRebootUpTime)%>';
		cbAutoWanPortIdle.checked				= ('<%ejGet(autoWanPortIdle)%>' == 1?true:false);
		arSetStartTimeHour.value				= '<%ejGet(autoRebootHRangeStartHour)%>';
		arSetStartTimeMin.value				= '<%ejGet(autoRebootHRangeStartMin)%>';
		arSetEndTimeHour.value				= '<%ejGet(autoRebootHRangeEndHour)%>';
		arSetEndTimeMin.value				= '<%ejGet(autoRebootHRangeEndMin)%>';
		nDay									= '<%ejGet(autoRebootDay)%>';
		txAutoRebootAvrData.value				= '<%ejGet(autoRebootAvrData)%>'; 
		txAutoRebootAvrDataEPG.value			= '<%ejGet(autoRebootAvrDataEPG)%>'; 
		for(var i=0; i<7; i++)
		{
			var it;
			if(it = (nDay & 0x01 << ((6-i)*4) ))
				cbAutoRebootDay[i].checked = true;
			else
				cbAutoRebootDay[i].checked = false;
		}
	}
}

function isAutoRebotEnable()
{
	if(varEnblAutoReboot == true)
	{
		if(varEnblAutoRebootStaticConfig == true)
		{
			return true;
		}
		else if((varAutoRebootOnIdle == true) && (varLdapConfigServer == true))
		{
			return true;
		}
	}
	return false;
}

function checkAutoRebootDay()
{
	var i =0;
	nDay =0;
	with ( document.forms[0] ) {

		for(i=0; i<7; i++)
		{
			if(cbAutoRebootDay[i].checked == true)
			{
				nDay = nDay | 0x01 << ((6-i)*4);
			}
		}
	}
}


function setSelectDay(nDay) {
	with ( document.forms[0] ) {
		nDayOfWeek = '';

		for(var i=0; i<7; i++)
		{
			nPart = '';
			if(nDayOfWeek != '\\')
				nPart += ',';
			if(cbSelDay[i].checked == true)
			{
				switch(i)
				{
					case 0:
						   nPart += '일';
						   break;
					case 1:
						   nPart += '월';
						   break;
					case 2:
						   nPart += '화';
						   break;
					case 3:
						   nPart += '수';
						   break;
					case 4:
						   nPart += '목';
						   break;
					case 5:
						   nPart += '금';
						   break;
					case 6:
						   nPart += '토';
						   break;
				}
			}
			else
				nPart ='';
			nDayOfWeek += nPart;
		}
	}
}

function checkTime() {
	with ( document.forms[0] ) {   
		if((arSetStartTimeHour.value == arSetEndTimeHour.value)&& 
			(arSetStartTimeMin.value == arSetEndTimeMin.value) &&
				(arSetStartTimeHour.value != 'NONE')&& 
				(arSetEndTimeHour.value != 'NONE') &&
				(arSetStartTimeMin.value != 'NONE')&& 
				(arSetEndTimeMin.value != 'NONE')) { 
			var startTime = document.getElementById('arSetStartTimeHour');
			var EndTime = document.getElementById('arSetEndTimeHour');
			startTime.options[0].selected = true;
			EndTime.options[23].selected = true;
			alert('시작시간과 끝나는 시간이 같습니다.');
		}
	}
}

function btnSave(obj)
{

	if (isAttackXSS(document.autoreboot.txAutoRebootUpTime.value)) {
		alert("ERROR: Please check the value again");
        document.autoreboot.txAutoRebootUpTime.focus();
        return false;
    }

	if (isAttackXSS(document.autoreboot.arSetStartTimeHour.value)) {
		alert("ERROR: Please check the value again");
        document.autoreboot.arSetStartTimeHour.focus();
        return false;
    }

	if (isAttackXSS(document.autoreboot.arSetStartTimeMin.value)) {
		alert("ERROR: Please check the value again");
        document.autoreboot.arSetStartTimeMin.focus();
        return false;
    }

	if (isAttackXSS(document.autoreboot.arSetEndTimeHour.value)) {
		alert("ERROR: Please check the value again");
        document.autoreboot.arSetEndTimeHour.focus();
        return false;
    }

	if (isAttackXSS(document.autoreboot.arSetEndTimeMin.value)) {
		alert("ERROR: Please check the value again");
        document.autoreboot.arSetEndTimeMin.focus();
        return false;
    }

	if (isAttackXSS(document.autoreboot.txAutoRebootAvrData.value)) {
		alert("ERROR: Please check the value again");
        document.autoreboot.txAutoRebootAvrData.focus();
        return false;
    }

	if (isAttackXSS(document.autoreboot.txAutoRebootAvrDataEPG.value)) {
		alert("ERROR: Please check the value again");
        document.autoreboot.txAutoRebootAvrDataEPG.focus();
        return false;
    }

    obj.isclick = 1;
    postTableEncrypt(document.autoreboot.postSecurityFlag, document.autoreboot);
	return true;
}

// done hiding -->
</SCRIPT>
</head>
<body>
<div class="intro_main ">
    <p class="intro_title"><% multilang(LANG_AUTO_REBOOT);%></p>
    <p class="intro_content"><%  %></p>
</div>

<form action=/boaform/admin/formAutoReboot method=POST name="autoreboot">
<div class="data_common data_common_notitle">
    <table>
		<p>자동 Reboot 설정</p>
		  <table>
			  <tr>
				<% ShowAutoSetting(); %>
			  </tr>
			  <tr>
				<% ShowRebootTargetTime(); %>
			  </tr>

		  </table>
	</table>
</div>

<div class="data_common data_common_notitle">
    <table>

		  <p>LDAP 설정</p>
		  <table>

			<% showCurrentAutoRebootConfigInfo(); %>
		</table>
	</table>
</div>

<div class="data_common data_common_notitle">
		<table>
		 
		 <p>수동 설정</p>
		  <table>
			<tr>
				<% ShowManualSetting(); %>
			</tr>
			<tr>
			  <td>자동 Reboot Up Time(일 단위)</td>
			  <td><input type='text' name='txAutoRebootUpTime' value=<% getInfo("txAutoRebootUpTime"); %>></td>
			</tr>
			<tr>
			  <td width="30%">자동 Reboot 시간(00~23)</td>
			  <td>
			<div class="data_common data_common_notitle">
				<table>
                <tr>

                <td > <input size="4" maxlength="2" type='text' id='arSetStartTimeHour' name='arSetStartTimeHour' value=<% getInfo("arSetStartTimeHour"); %>></td>
                <td >시</td>

                <td > <input size="4" maxlength="2" type='text' id='arSetStartTimeMin' name='arSetStartTimeMin' value=<% getInfo("arSetStartTimeMin"); %>></td>
                <td >분</td>
                <td>~</td>

                <td > <input size="4" maxlength="2" type='text' id='arSetEndTimeHour' name='arSetEndTimeHour' value=<% getInfo("arSetEndTimeHour"); %>></td>
                <td >시</td>
                <td > <input size="4" maxlength="2" type='text' id='arSetEndTimeMin' name='arSetEndTimeMin' value=<% getInfo("arSetEndTimeMin"); %>></td>
                <td >분</td>
                </tr>
              </table>
			</div>
			</td>
			</tr>

			  <tr>
				<% ShowAutoRebootDay(); %>
			  </tr>

			  <tr>
				<% ShowAutoWanPortIdleValue(); %>
			  </tr>
			  <tr>
				  <td>평균 데이터량</td> 
				  <td> <input type='text' name='txAutoRebootAvrData' value=<% getInfo("txAutoRebootAvrData"); %>> 
				  </td>
			  </tr>
			  <tr>
				  <td>평균 데이터량(EPG)</td> 
				  <td> <input type='text' name='txAutoRebootAvrDataEPG' value=<% getInfo("txAutoRebootAvrDataEPG"); %>> 
				  </td>
			  </tr>
		  </table>

	</table>
</div>

	<div class="btn_ctl">
		<input type="hidden" value="/admin/autoreboot.asp" name="submit-url">
		<input type="submit" name="addAutoRebootStaticConfig" value="<% multilang(LANG_APPLY); %>" class="link_bg" onClick="return btnSave(this)">
		<input type="hidden" name="postSecurityFlag" value="">
	</div>


  </form>
</body>
</html>
