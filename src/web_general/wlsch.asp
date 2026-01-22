<%SendWebHeadStr(); %>
<title><% multilang(LANG_WLAN_SCHEDULE_SETTING); %></title>

<script>
var maxScheduleNum = <% checkWrite("maxWebWlSchNum"); %>;
var wlschMode= <% checkWrite("wlsch_mode"); %>;
var wlschOnOff=	<% checkWrite("wlsch_Onoff"); %>;

function wlsch_from_init(index, value)
{
	var starthours=Math.floor(parseInt(value, 10)/60);
	//alert("starthours="+starthours);
	var startmin=parseInt(value, 10)%60;
	//alert("startmin="+startmin);
	get_by_id("from_hour_"+index).value = starthours;
	get_by_id("from_min_"+index).value = startmin;		
}

function wlsch_to_init(index, value)
{
	var endhours=Math.floor(parseInt(value, 10)/60);
	//alert("endhours="+endhours);
	var endmin=parseInt(value, 10)%60;
	//alert("endmin="+endmin);
	get_by_id("to_hour_"+index).value = endhours;
	get_by_id("to_min_"+index).value = endmin;		
}

function wlsch_action_init(index, value)
{
	get_by_id("action_"+index).value = value;
}
function wlsch_from_select(index)
{
	var starthours=get_by_id("from_hour_"+index).value;
	//alert("starthours="+starthours);
	var startmin=get_by_id("from_min_"+index).value;
	//alert("startmin="+startmin);
	starthours=Number(starthours);
	startmin=Number(startmin);
	get_by_id("wlsch_from_"+index).value = starthours*60 + startmin;
}

function wlsch_to_select(index)
{
	var tohours=get_by_id("to_hour_"+index).value;
	//alert("tohours="+tohours);
	var tomin=get_by_id("to_min_"+index).value;
	//alert("tomin="+tomin);
	tohours=Number(tohours);
	tomin=Number(tomin);
	get_by_id("wlsch_to_"+index).value = tohours*60+tomin;
}

function wlsch_action_select(index)
{
	var action=get_by_id("action_"+index).value;
	get_by_id("wlsch_action_"+index).value =action;
}
function wlsch_day_select(index, value)
{
	if(value == 1 || value == true)
	{
		get_by_id("wlsch_enable_"+index).value = 1;	
		get_by_id("enable_"+index).checked = true;
		
	}
	else
	{
		get_by_id("wlsch_enable_"+index).value = 0;
		get_by_id("enable_"+index).checked = false;
		
	}
}

function wlsch_weekday_select(index, value)
{
	get_by_id("wlsch_day_"+index).value = value;
	get_by_id("day_"+index).value = value;
}

function wlsch_onoff_select(value)
{
	var i =0;	
	if(value == 1 || value == true)
	{
		get_by_id("wlsch_onoff").value = 1;
		//get_by_id("onoff").value = 1;
		//get_by_id("onoff").checked = true;
	
		for(i=0;i<maxScheduleNum;i++)
		{
			get_by_id("enable_"+i).disabled = false;
			//get_by_id("day_"+i).disabled = false;
			get_by_id("from_hour_"+i).disabled = false;
			get_by_id("from_min_"+i).disabled = false;
			if(document.mainform.wlsch_mode.value==1){
				get_by_id("action_"+i).disabled = false;
				get_by_id("to_hour_"+i).disabled = true;
				get_by_id("to_min_"+i).disabled = true;
			}else{
				get_by_id("action_"+i).disabled = true;
				get_by_id("to_hour_"+i).disabled = false;
				get_by_id("to_min_"+i).disabled = false;
			}
		}
	}
	else{
		get_by_id("wlsch_onoff").value = 0;
		//get_by_id("onoff").value = 0;
		//get_by_id("onoff").checked = false;
		
		for(i=0;i<maxScheduleNum;i++)
		{
			get_by_id("enable_"+i).disabled = true;
			//get_by_id("day_"+i).disabled = true;
			get_by_id("from_hour_"+i).disabled = true;
			get_by_id("from_min_"+i).disabled = true;
			get_by_id("action_"+i).disabled = true;
			get_by_id("to_hour_"+i).disabled = true;
			get_by_id("to_min_"+i).disabled = true;	
		}
	}
}
function init()
{
	var i=0;
	var f=document.mainform;
	if(wlschOnOff==1){
		document.mainform.onoff.value=1;
		document.mainform.onoff.checked=true;
		if(wlschMode==1){
			document.mainform.wlsch_mode.value=1;
			document.mainform.wlsch_mode.checked=true;
		}else{
			document.mainform.wlsch_mode.value=0;
			document.mainform.wlsch_mode.checked=false;
		}
	}else{
		document.mainform.onoff.value=0;
		document.mainform.onoff.checked=false;
		document.mainform.wlsch_mode.value=0;
		document.mainform.wlsch_mode.checked=false;
	}

	for(i=0;i<maxScheduleNum;i++)
	{
		//alert(get_by_id("wlsch_from_"+i).value);
		wlsch_from_init(i, get_by_id("wlsch_from_"+i).value);	
		wlsch_to_init(i, get_by_id("wlsch_to_"+i).value);
		wlsch_action_init(i,get_by_id("wlsch_action_"+i).value);
		wlsch_day_select(i, get_by_id("wlsch_enable_"+i).value);
		wlsch_weekday_select(i, get_by_id("wlsch_day_"+i).value);

		if(document.mainform.onoff.checked==true)
		{
			get_by_id("enable_"+i).disabled = false;
			//get_by_id("day_"+i).disabled = false;
			get_by_id("from_hour_"+i).disabled = false;
			get_by_id("from_min_"+i).disabled = false;
			if(document.mainform.wlsch_mode.checked==true)
			{
				get_by_id("action_"+i).disabled = false;
				get_by_id("to_hour_"+i).disabled = true;
				get_by_id("to_min_"+i).disabled = true;
			}else{
				get_by_id("action_"+i).disabled = true;
				get_by_id("to_hour_"+i).disabled = false;
				get_by_id("to_min_"+i).disabled = false;
			}
		}else{
			get_by_id("enable_"+i).disabled = true;
			//get_by_id("day_"+i).disabled = true;
			get_by_id("from_hour_"+i).disabled = true;
			get_by_id("from_min_"+i).disabled = true;
			get_by_id("action_"+i).disabled = true;
			get_by_id("to_hour_"+i).disabled = true;
			get_by_id("to_min_"+i).disabled = true;	
		}
	}

	//wlsch_onoff_select(get_by_id("onoff").checked);	

}

function updateAction(value)
{
	var i=0;
 	if (value==1 || value==true) {
		document.mainform.wlsch_mode.value=1;
		
		for(i=0;i<maxScheduleNum;i++)
  		{
  			wlsch_to_init(i, 0);
			wlsch_action_init(i,get_by_id("wlsch_action_"+i).value);
			get_by_id("action_"+i).disabled = false;
			get_by_id("to_hour_"+i).disabled = true;
			get_by_id("to_min_"+i).disabled = true;
		}
  	}else {
		document.mainform.wlsch_mode.value=0;
		
		for(i=0;i<maxScheduleNum;i++)
		{
			wlsch_to_init(i, get_by_id("wlsch_to_"+i).value);
			wlsch_action_init(i,0);
			get_by_id("action_"+i).disabled = true;
			get_by_id("to_hour_"+i).disabled = false;
			get_by_id("to_min_"+i).disabled = false;
		}
	}
}

	var token= new Array();		
	var DataArray = new Array();
	token[0]='<% wlSchList("wlSchList_0");%>';
	token[1]='<% wlSchList("wlSchList_1");%>';
	token[2]='<% wlSchList("wlSchList_2");%>';
	token[3]='<% wlSchList("wlSchList_3");%>';
	token[4]='<% wlSchList("wlSchList_4");%>';
	token[5]='<% wlSchList("wlSchList_5");%>';
	token[6]='<% wlSchList("wlSchList_6");%>';
	token[7]='<% wlSchList("wlSchList_7");%>';	
	token[8]='<% wlSchList("wlSchList_8");%>';	
	token[9]='<% wlSchList("wlSchList_9");%>';
	token[10]='<% wlSchList("wlSchList_10");%>';
	token[11]='<% wlSchList("wlSchList_11");%>';
	token[12]='<% wlSchList("wlSchList_12");%>';
	token[13]='<% wlSchList("wlSchList_13");%>';
	token[14]='<% wlSchList("wlSchList_14");%>';
	token[15]='<% wlSchList("wlSchList_15");%>';
	token[16]='<% wlSchList("wlSchList_16");%>';
	token[17]='<% wlSchList("wlSchList_17");%>';	
	token[18]='<% wlSchList("wlSchList_18");%>';	
	token[19]='<% wlSchList("wlSchList_19");%>';
	token[20]='<% wlSchList("wlSchList_20");%>';
	token[21]='<% wlSchList("wlSchList_21");%>';
	token[22]='<% wlSchList("wlSchList_22");%>';
	token[23]='<% wlSchList("wlSchList_23");%>';
	token[24]='<% wlSchList("wlSchList_24");%>';
	token[25]='<% wlSchList("wlSchList_25");%>';
	token[26]='<% wlSchList("wlSchList_26");%>';
	token[27]='<% wlSchList("wlSchList_27");%>';	
	token[28]='<% wlSchList("wlSchList_28");%>';	
	token[29]='<% wlSchList("wlSchList_29");%>';
	token[30]='<% wlSchList("wlSchList_30");%>';	
	token[31]='<% wlSchList("wlSchList_31");%>';

function wlSchList(num)
{
	var strTmp;
	var j=0;
		
	for (var i = 0; i < num; i++)
	{
		/* eco/day/fTime/tTime/week */
		DataArray = token[i].split("|");

		document.write("<tr class=\"tbl_body\"><td height='30' align=center width='10%'><font size='2'>");
		document.write("<input type='hidden' id='wlsch_enable_"+i+"' name='wlsch_enable_"+i+"' value='"+DataArray[0]+"'*1>");
		document.write("<input type='checkbox' id='enable_"+i+"' onclick='wlsch_day_select("+i+",this.checked);'></td>");

		document.write("<td height='30'' align=center width='15%'><font size='2'>");				      	
      	document.write("<input type='hidden' id='wlsch_day_"+i+"' name='wlsch_day_"+i+"' value='"+DataArray[1]+"'*1>");

		document.write("<select id='day_"+i+"' size='1' onchange='wlsch_weekday_select("+i+", this.value);'*1>");
			//dispalyWeekDayOption();
			document.write("<option value='0'><% multilang(LANG_WLAN_SCHEDULE_SUN);%></option>");
			document.write("<option value='1'><% multilang(LANG_WLAN_SCHEDULE_MON);%></option>");
			document.write("<option value='2'><% multilang(LANG_WLAN_SCHEDULE_TUE);%></option>");
			document.write("<option value='3'><% multilang(LANG_WLAN_SCHEDULE_WED);%></option>");
			document.write("<option value='4'><% multilang(LANG_WLAN_SCHEDULE_THU);%></option>");
			document.write("<option value='5'><% multilang(LANG_WLAN_SCHEDULE_FRI);%></option>");
			document.write("<option value='6'><% multilang(LANG_WLAN_SCHEDULE_SAT);%></option>");
			document.write("<option value='7'><% multilang(LANG_WLAN_SCHEDULE_EVERYDAY);%></option>");
			document.write("		</select> </font></td>");
			
      document.write("<td align=center width='25%''><font size='2'>");
      document.write("<input type='hidden' id='wlsch_from_"+i+"' name='wlsch_from_"+i+"' value='"+DataArray[2]+"'*1>");

      document.write("<select id='from_hour_"+i+"' size='1' onchange='wlsch_from_select("+i+");'*1>");
				//dispalyFromHourOption();
			for(j = 0; j < 24; j++){
				if(j<10){
					strTmp ="<option value="+j+">"+0+j+"</option>";
				}
				else{
					strTmp ="<option value="+j+">"+j+"</option>";
				}
				document.write(strTmp);
			}	
			
      document.write("</select><% multilang(LANG_WLAN_SCHEDULE_FOR_HOUR);%>");
      document.write("<select id='from_min_"+i+"' size='1' onchange='wlsch_from_select("+i+");'*1>");
          	//dispalyFromMinOption();
		for(j = 0; j < 60; j++){
				if(j<10){
					strTmp ="<option value="+j+">"+0+j+"</option>";
				}
				else{
					strTmp ="<option value="+j+">"+j+"</option>";
				}
				document.write(strTmp);
		}	
      document.write("</select>	<% multilang(LANG_WLAN_SCHEDULE_FOR_MIN);%>	</font></td>");

		document.write("<td align=center width='25%'><font size='2'>");
		document.write("<input type='hidden' id='wlsch_to_"+i+"' name='wlsch_to_"+i+"' value='"+DataArray[3]+"'*1>");
		document.write("<select id='to_hour_"+i+"' size='1' onchange='wlsch_to_select("+i+");'*1>");
          	//dispalyFromHourOption();
          	for(j = 0; j < 24; j++){
				if(j<10){
					strTmp ="<option value="+j+">"+0+j+"</option>";
				}
				else{
					strTmp ="<option value="+j+">"+j+"</option>";
				}
				document.write(strTmp);
			}
		document.write("</select>	<% multilang(LANG_WLAN_SCHEDULE_FOR_HOUR);%>");
		document.write("<select id='to_min_"+i+"' size='1' onchange='wlsch_to_select("+i+");'*1>");
          	//dispalyFromMinOption();
          	for(j = 0; j < 60; j++){
				if(j<10){
					strTmp ="<option value="+j+">"+0+j+"</option>";
				}
				else{
					strTmp ="<option value="+j+">"+j+"</option>";
				}
				document.write(strTmp);
			}	
		document.write("</select>	<% multilang(LANG_WLAN_SCHEDULE_FOR_MIN);%>	</font></td>");
			
		document.write("<td height='30' align=center width='25%'><font size='2'>");
        document.write("<input type='hidden' id='wlsch_action_"+i+"' name='wlsch_action_"+i+"' value='"+DataArray[4]+"'*1>");

        document.write("<select id='action_"+i+"' size='1' onchange='wlsch_action_select("+i+");'>");
          	//dispalyActionOption();
		document.write("<option value='0'><% multilang(LANG_WLAN_SCHEDULE_ACTION_DISABLE);%></option>");
		document.write("<option value='1'><% multilang(LANG_WLAN_SCHEDULE_ACTION_ENABLE);%></option>");
	
        document.write("    </select><% multilang(LANG_WLAN_SCHEDULE_FOR_ACTION);%>	</font></td></tr>");				
  	}
}
function save_change(obj)
{
	obj.isclick = 1;
	postTableEncrypt(document.mainform.postSecurityFlag, document.mainform);
	return true;
}
function page_reset()
{	
	//updateAction();	
	for(var i=0;i<maxScheduleNum;i++)
	{
		DataArray = token[i].split("|");
		//alert(get_by_id("wlsch_from_"+i).value);
		wlsch_day_select(i, DataArray[0]);
		wlsch_weekday_select(i, DataArray[1]);
		wlsch_from_init(i, DataArray[2]);
		wlsch_to_init(i, DataArray[3]);
		wlsch_action_init(i, DataArray[4]);
	}
	DataArray = token[0].split("|");
	
	wlsch_onoff_select(get_by_id("wlsch_onoff").value);
}
function page_submit(obj)
{
/*
	alert("value submit value"+document.mainform.wlsch_mode.value);
	if(document.mainform.wlsch_mode.value==0){
		for(index=0;index<maxScheduleNum;index++)
		{
			if(get_by_id("wlsch_from_"+index).value > get_by_id("wlsch_to_"+index).value)
			{
				alert("input rule error"+index);
				return false;
			}
			if(get_by_id("enable_"+index).checked == true)
			{
				if(get_by_id("wlsch_from_"+index).value == get_by_id("wlsch_to_"+index).value)
				{
					alert("input rule error"+index);
					return false;
				}
			}
		}
	}
	*/
	obj.isclick = 1;
	postTableEncrypt(document.mainform.postSecurityFlag, document.mainform);
	document.forms.mainform.submit();
	return true;	
}
</script>
</head>
  
 <body>
<div class="intro_main ">
	<p class="intro_title"><% multilang(LANG_WLAN_SCHEDULE_HEADER); %></p>
	<p class="intro_content"><% multilang(LANG_WLAN_SCHEDULE_EXPLAIN); %></p>
	<p class="intro_content"><% multilang(LANG_WLAN_SCHEDULE_EXPLAIN_MODE0); %></p>
	<p class="intro_content"><% multilang(LANG_WLAN_SCHEDULE_EXPLAIN_MODE1); %></p>
<!-- <tr><td><hr size="1" align="top" noshade="noshade"></td></tr>  -->
</div>

<form action=/boaform/formNewSchedule method=POST name="mainform">
  
<div class="data_common data_common_notitle">	
   <table border=0 width="660" cellspacing=4 cellpadding=0>
   <tr><td><font size=2><b>
    		<input type="hidden" id="wlsch_onoff" name="wlsch_onoff" value="<% checkWrite("wlsch_Onoff"); %>">
		   	<input type="checkbox" name="onoff"  value="true" onClick="wlsch_onoff_select(this.checked);">&nbsp;&nbsp;<% multilang(LANG_WLAN_SCHEDULE_ENABLE);%></b><br>
		</td>
    </tr> 

	<tr><td><font size=2><b>
			<input type="checkbox" name="wlsch_mode" value="true" ONCLICK="updateAction(this.checked)">&nbsp;&nbsp;<% multilang(LANG_WLAN_SCHEDULE_MODE_ENABLE);%></b>
		</td>
	</tr>
	
  </table>
</div>
<div class="data_common data_common_notitle">
  <table border="0" width=660>
  
<tr class="tbl_head">
  	<td height="30" align=center width="10%" ><font size="2"><b><% multilang(LANG_WLAN_SCHEDULE_ENA);%></b></font></td>
  	<td height="30" align=center width="15%" ><font size="2"><b><% multilang(LANG_WLAN_SCHEDULE_DAY);%></b></font></td>
  	<td align=center width="25%" ><font size="2"><b><% multilang(LANG_WLAN_SCHEDULE_FROM);%></b></font></td>
  	<td align=center width="25%" ><font size="2"><b><% multilang(LANG_WLAN_SCHEDULE_TO);%></b></font></td>
  	<td height="30" align=center width="25%" ><font size="2"><b><% multilang(LANG_WLAN_SCHEDULE_FROM_ACTION);%></b></font></td>	
</tr>
	
<script>
	wlSchList(maxScheduleNum);
</script>
<!--
	<tr><td>&nbsp;</td></tr>
-->			
  </table>
  
</div>
  <br />
  <div class="btn_ctl">
    <input type="hidden" id="save_apply_flag" name="save_apply_flag" value="1">
    <input type="submit" name="addScl" value="<% multilang(LANG_WLAN_SCHEDULE_APPLY);%>" onclick="return save_change(this);">&nbsp;&nbsp;
 	<input type="button" name="Reset" value="<% multilang(LANG_WLAN_SCHEDULE_RESET);%>" onclick="return page_reset();">
 	<input type="hidden" name="wlan_idx" value=<% checkWrite("wlan_idx"); %>>
  	<input type="hidden" value="/wlsch.asp" name="submit-url">
	<input type="hidden" name="postSecurityFlag" value="">
  </div>
  
 <script>init();</script>

	</form>
 	<br>
  <br>
  <br>
</blockquote> 
  </body></html>
