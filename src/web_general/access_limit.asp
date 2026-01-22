<% SendWebHeadStr();%>
<title>인터넷 접속제한 서비스 <% multilang(LANG_CONFIGURATION); %></title>

<SCRIPT>
var ifnum;

var nDayOfWeek;

function btnSave(obj) 
{

	if (isAttackXSS(document.accesslimit.hostName.value)) {
		alert("ERROR: Please check the value again");
		document.accesslimit.hostName.focus();
		return false;
	}

	if (isAttackXSS(document.accesslimit.hostAddress.value)) {
		alert("ERROR: Please check the value again");
		document.accesslimit.hostAddress.focus();
		return false;
	}

	obj.isclick = 1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);

	return true;
}

function setSelectDay(nDay) {
	with ( document.forms[0] ) {
		nDayOfWeek = '';
		if(nDay == 0 && cbSelDay[7].checked == true){
			for(var i=0; i<8; i++){
				cbSelDay[i].checked = true;
			}
		}
		else if( (cbSelDay[0].checked != true)|| 
				(cbSelDay[1].checked != true)|| 
				(cbSelDay[2].checked != true)|| 
				(cbSelDay[3].checked != true)|| 
				(cbSelDay[4].checked != true)|| 
				(cbSelDay[5].checked != true)|| 
				(cbSelDay[6].checked != true)){ 
			cbSelDay[7].checked = false;
		}
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
						   nPart += 'Sun';
						   break;
					case 1:
						   nPart += 'Mon';
						   break;
					case 2:
						   nPart += 'Tue';
						   break;
					case 3:
						   nPart += 'Wed';
						   break;
					case 4:
						   nPart += 'Thu';
						   break;
					case 5:
						   nPart += 'Fri';
						   break;
					case 6:
						   nPart += 'Sat';
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
       if((setStartTimeHour.value == setEndTimeHour.value)&& 
       (setStartTimeHour.value != 'NONE')&& 
       (setStartTimeHour.value != 'NONE')){ 
           var startTime = document.getElementById('setStartTimeHour');
           var EndTime = document.getElementById('setEndTimeHour');
           startTime.options[0].selected = true;
           EndTime.options[23].selected = true;
           alert('시작시간과 끝나는 시간이 같습니다.');
       }
   }
}

function setMac(){
	with ( document.forms[0] ) 
	{   
		if(document.accesslimit.selectMac.value != 'NONE')
		{
			document.accesslimit.hostAddress.value = document.accesslimit.selectMac.value;
			document.accesslimit.hostAddress.focus();
		}
	}

	return true;
}

function usedelClick(obj)
{
	obj.isclick = 1;
	postTableEncrypt(document.accesslimit.postSecurityFlag, document.accesslimit);
	return true;
}

// done hiding -->
</SCRIPT>
</head>

<body>
<div class="intro_main ">
	<p class="intro_title">인터넷 접속제한 서비스 <% multilang(LANG_CONFIGURATION); %></p>
	<p class="intro_content"></p>
</div>

<form action=/boaform/admin/formAccessLimit method=POST name="accesslimit">
<div class="data_common data_common_notitle">

	<table>
    <tr>
    <td>
		<div class="column_title">
			<div class="column_title_left"></div>
				<p>설정 추가</p>
		</div>
		<% showCurrentTime(); %>

		<table>
        <tr>
            <th width='17%%'>단말이름</th>
            <td width='33%%'><input type='text' name='hostName'></td>
            <th width='17%%'>허용/차단</th>
            <td width='33%%'>
            <select name='selectAllow' size='1'>
				<option value='1'>차단
				<option value='0'>허용
            </select>
            </td>
        </tr>
        <tr>
            <th>단말주소(MAC)</th>
            <td><input type='text' name='hostAddress'></td>
            <th>단말찾기</th>
            <td>
            <select name='selectMac' onClick='setMac()' size='1'>
				<option value='NONE'>단말 선택 안함
				<% getConnectHostList(); %>
            </select>
            </td>
        </tr>

        <tr>
        </tr>
		</table>

		<table>
        <tr>
            <th>일정</th>
            <td>
<!--
                <input type='checkbox' name='cbSelDay0' onClick='setSelectDay(1)' value='1'>일&nbsp;&nbsp;&nbsp;
                <input type='checkbox' name='cbSelDay1' onClick='setSelectDay(2)' value='2'>월&nbsp;&nbsp;&nbsp;
                <input type='checkbox' name='cbSelDay2' onClick='setSelectDay(3)' value='4'>화&nbsp;&nbsp;&nbsp;
                <input type='checkbox' name='cbSelDay3' onClick='setSelectDay(4)' value='8'>수&nbsp;&nbsp;&nbsp;
                <input type='checkbox' name='cbSelDay4' onClick='setSelectDay(5)' value='16'>목&nbsp;&nbsp;&nbsp;
                <input type='checkbox' name='cbSelDay5' onClick='setSelectDay(6)' value='32'>금&nbsp;&nbsp;&nbsp;
                <input type='checkbox' name='cbSelDay6' onClick='setSelectDay(7)' value='64'>토&nbsp;&nbsp;&nbsp;
                <input type='checkbox' name='cbSelDay7' onClick='setSelectDay(0)' value='128'>매일&nbsp;&nbsp;&nbsp;
-->
                <input type='checkbox' name='cbSelDay0' value='1'>일&nbsp;&nbsp;&nbsp;
                <input type='checkbox' name='cbSelDay1' value='2'>월&nbsp;&nbsp;&nbsp;
                <input type='checkbox' name='cbSelDay2' value='4'>화&nbsp;&nbsp;&nbsp;
                <input type='checkbox' name='cbSelDay3' value='8'>수&nbsp;&nbsp;&nbsp;
                <input type='checkbox' name='cbSelDay4' value='16'>목&nbsp;&nbsp;&nbsp;
                <input type='checkbox' name='cbSelDay5' value='32'>금&nbsp;&nbsp;&nbsp;
                <input type='checkbox' name='cbSelDay6' value='64'>토&nbsp;&nbsp;&nbsp;
                <input type='checkbox' name='cbSelDay7' value='128'>매일&nbsp;&nbsp;&nbsp;
            </td>
        </tr>
        <tr>
            <th>시간</th>
            <td>
            <table> 
                <tr>
                <td>
                <select class='form-control' id='setStartTimeHour' name='setStartTimeHour' size='1' onChange="checkTime()">       
				<% hourTimeSet(); %>
                </select>
                </td>

                <td width='50'>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;:
                </td>

                <td>
                <select class='form-control' id='setStartTimeMin' name='setStartTimeMin' size='1' onChange="checkTime()">       
				<% minTimeSet(); %>
                </select>
                </td>
                <td width='50'>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;~
                </td>
                <td>
				<select class='form-control' id='setEndTimeHour' name='setEndTimeHour' size='1' onChange="checkTime()">       
				<% hourTimeSet(); %>
                </select>
                </td>
                <td width='50'>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;:
                </td>

                <td>
                <select class='form-control' id='setEndTimeMin' name='setEndTimeMin' size='1' onChange="checkTime()">     
				<% minTimeSet(); %>
                </select>
                </td>
                </tr>
                </table>
                </td>
                </tr>

                </table>

				<div class="btn_ctl">
					<input type="submit" name="addAccessList" value="추가" class="link_bg" onClick="return btnSave(this)">
				</div>

                <br>

				<div class="column_title">
					<div class="column_title_left"></div>
						<p>인터넷 접속제한 리스트</p>
					<div class="column_title_right"></div>
				</div>

                   <table>
                       <tr>
                           <th align="center">순서</th>
                           <th align="center">단말 이름</th>
                           <th align="center">사용</th>
                           <th align="center">단말주소(MAC)</th>
                           <th align="center">요일/시간</th>
                           <th align="center">허용/차단</th>
                           <th align="center">삭제</th>
                       </tr>
					   <% showCurrentAccessList(); %>
					</table>
				<div class="btn_ctl">
					<input type="submit" name="usedelAccessList" value="<% multilang(LANG_APPLY); %>" class="link_bg" onClick="return usedelClick(this)">
				</div>
			</td>
		</tr>
   </table>
   </div>

<div class="column">
   <input type="hidden" value="/admin/access_limit.asp" name="submit-url">
   <input type="hidden" name="postSecurityFlag" value="">
</div>

   </form>
</body>
</html>
