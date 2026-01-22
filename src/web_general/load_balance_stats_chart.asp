<% SendWebHeadStr();%>
<title><% multilang(LANG_LOAD_BALANCE_STATS); %></title>

<style type="text/css">
.progress_container_1{	
	margin-left:0px;
	width: 300px;
	height: 25px;
	border:solod 1px #008CBA;

	text-align: left;
	font-size: 14px;
}

.progress_container_2{width: 300px;}

.progress{
	height: 100%;
	background-color:#008CBA;
	border-radius: 4px;
	
	text-align: right;
	padding-top: 6px;
	font-size: 14px;
}

ul {
	margin:0; 
	padding:0; 
	float:left; 
	align:center; 
}

ul li {
	display:inline-block; 
	list-style:none; 
	text-align:center; 
	align:center; 
	margin:10px; 
	height:20px; 
}

ul li p {
	margin:5px; 
	font-size: 12px;
}

table{ 
	border-collapse:collapse;
}

table tr td{ 
	border-bottom:2px solid #848484; 
	font-size: 12px; 
	text-align:center;
} 

table th{ 
	height:25px;  
	border-top:2px; 
	border-bottom:2px solid #848484; 
	font-size: 14px; 
	background-color: #008CBA;
} /*008CBA*/

</style>

<script>
var AjaxReq = null;
var AjaxReqSend = 0;

var ratio_process = 0.5;
var scale_factor = 0.7;

var current_wan_nums = <%rtk_load_balance_wan_nums(); %>;
var total_wans_connect_status = 0;

function parseAjaxResponse(string_text){
	var per_wan = new Array();
	var per_wan_val = new Array();
	
	var total_rate = 0;
	var total_dev_nums = 0;

	per_wan = string_text.split(",");
	if (per_wan.length > 0)
		total_wans_connect_status = 0;

	for (i=0; i<per_wan.length; i++) {
		var per_wan_name = "";
		var per_wan_state = "disconnected";
		var per_wan_rate = 0;
		var per_wan_rate_percentage = "0%";
		var per_wan_dev_nums = 0;
		var per_wan_onoff_state = "off";

	//	console.log("per_wan:", per_wan[i]);
		per_wan_val = per_wan[i].split("|");
		for (j =0; j<per_wan_val.length; j++){
			var tmp_str = per_wan_val[j];
			var tmp_str_len = tmp_str.length;
			var tmp_idx = tmp_str.indexOf("=");
			var tmp_val = tmp_str.substr(tmp_idx+1, tmp_str_len);
			
		//	 console.log("tmp_str:", tmp_str);
			
			if (tmp_str.indexOf("wan=") != -1){		
				per_wan_name = tmp_val;
		//		console.log("per_wan_name:", per_wan_name);

			}else if (tmp_str.indexOf("state=") != -1){
				per_wan_state = tmp_val;
		//		console.log("per_wan_state:", per_wan_state);
				
			}else if (tmp_str.indexOf("rate=") != -1){	
				per_wan_rate = parseFloat(tmp_val);
		//		console.log("per_wan_rate:", per_wan_rate);
				total_rate += per_wan_rate;

			}else if (tmp_str.indexOf("percentage=") != -1){
				per_wan_rate_percentage = tmp_val;
		//		console.log("per_wan_rate_percentage:", per_wan_rate_percentage);

			}else if (tmp_str.indexOf("dev_nums=") != -1){
				per_wan_dev_nums = parseInt(tmp_val);
		//		console.log("per_wan_dev_nums:", per_wan_dev_nums);						
				total_dev_nums += per_wan_dev_nums;

			}else if (tmp_str.indexOf("on/off=") != -1){
				per_wan_onoff_state = tmp_val;
		//		console.log("per_wan_onoff_state:", per_wan_onoff_state);	

			}

			if (j == per_wan_val.length-1 && per_wan_name != ""){
				update_per_wan(per_wan_name, per_wan_state, per_wan_rate, per_wan_rate_percentage, per_wan_dev_nums, per_wan_onoff_state)
			}
		}
	//	console.log("total_rate:", total_rate);
	//	console.log("total_dev_nums:", total_dev_nums);
	}

	if (per_wan.length != 0){
		if (total_wans_connect_status > 0){
			var wan_rate_img_bar;
			var wan_total_rate_radio = 0;		
		//	console.log("current_wan_nums", parseInt(current_wan_nums));
			for(i=0; i < parseInt(current_wan_nums); i++){
				wan_rate_img_bar = document.getElementById("wan"+(i+1)+"_rate"+"_img");
				wan_total_rate_radio += parseInt(wan_rate_img_bar.style.width);
		//		console.log("i",i);
		//		console.log("width",wan_total_rate_radio);
			}	
		//	wan_total_rate_radio = wan_total_width/300*100;
			
		//	console.log("wan_total_rate_radio", wan_total_rate_radio);

			var total_rate_percentage = wan_total_rate_radio +'%';
			total_rate = total_rate.toFixed(1);
			update_wan_total(total_rate, total_rate_percentage, total_dev_nums);
		}else{
			update_wan_total("0.0", "0%", total_dev_nums);
		}		
	}
}

function update_per_wan(wan_id, state, rate, percentage, dev_nums, onoff){
	var wan_state_img_bar = document.getElementById(wan_id+"_state"+"_img");
	var wan_state_text_bar = document.getElementById(wan_id+"_state"+"_text");
	var wan_rate_img_bar = document.getElementById(wan_id+"_rate"+"_img");
	var wan_rate_img_span_bar = document.getElementById(wan_id+"_rate"+"_img"+"_span");
	var wan_dev_nums_text_bar = document.getElementById(wan_id+"_dev_nums"+"_text");
	var wan_onoff_img_bar = document.getElementById(wan_id+"_onoff"+"_img");
	var wan_onoff_text_bar = document.getElementById(wan_id+"_onoff"+"_text");

	//var new_percentage = parseFloat(percentage.replace('%', ''))+"%";

	if (state == "connected"){
		/*state*/
		wan_state_img_bar.src = "./graphics/icon/wan_connected.png";
		wan_state_text_bar.innerHTML = "Connected";
		wan_state_text_bar.style.color = "#0072BF";

		/*rate*/
		wan_rate_img_bar.style.width = percentage;
		wan_rate_img_span_bar.innerHTML = rate.toFixed(1) + "Mbps";

	//	console.log("connected rate:", rate);

		/*dev nums*/
		wan_dev_nums_text_bar.innerHTML = "x "+dev_nums;
		total_wans_connect_status |= 1;

	}else if(state == "disconnected"){
		/*state*/
		wan_state_img_bar.src = "./graphics/icon/wan_disconnected.png";
		wan_state_text_bar.innerHTML = "Disconnected";
		wan_state_text_bar.style.color = "#737373";

		/*rate*/
		wan_rate_img_bar.style.width = "0%";
		wan_rate_img_span_bar.innerHTML = "0.0Mbps";	

		/*dev nums*/
		wan_dev_nums_text_bar.innerHTML = "x "+dev_nums;		
	}

	if(onoff == "on"){
		/*on-off switch*/
		wan_onoff_img_bar.src = "./graphics/icon/wan_on.png";
		wan_onoff_text_bar.innerHTML = "On";
		wan_onoff_text_bar.style.color = "#229954";
	}else if(onoff == "off"){
		/*on-off switch*/
		wan_onoff_img_bar.src = "./graphics/icon/wan_off.png";
		wan_onoff_text_bar.innerHTML = "Off";
		wan_onoff_text_bar.style.color = "#707070";
	}
}

function update_wan_total(rate, percentage, dev_nums){
	var total_state_img_bar = document.getElementById("total"+"_state"+"_img");
	var total_state_text_bar =document.getElementById("total"+"_state"+"_text");
	var total_rate_img_bar = document.getElementById("total"+"_rate"+"_img");
	var total_rate_img_span_bar = document.getElementById("total"+"_rate"+"_img"+"_span");
	var total_dev_nums_text_bar = document.getElementById("total"+"_dev_nums"+"_text");

	if (total_wans_connect_status > 0){
		total_state_img_bar.src = "./graphics/icon/total_connected.png";
		total_state_text_bar.innerHTML = "Connected";//Connected&nbsp;&nbsp;&nbsp
		total_state_text_bar.style.color = "#0072BF";		
	}else {
		total_state_img_bar.src = "./graphics/icon/total_disconnected.png";
		total_state_text_bar.innerHTML = "Disconnected";
		total_state_text_bar.style.color = "#737373";
	}

	/*rate*/
	total_rate_img_bar.style.width = percentage;
	total_rate_img_span_bar.innerHTML = rate + "Mbps";

	/*dev nums*/
	total_dev_nums_text_bar.innerHTML = "x "+dev_nums;
}

function update_page()
{
	if (AjaxReq != null && AjaxReq.readyState==4 && AjaxReq.status==200)
	{
		var str_text = AjaxReq.responseText;
	//	console.log("string_text:", str_text);
		parseAjaxResponse(str_text);

		AjaxReqSend  = 0;
	}
}

function createRequest()
{
	var request = null;
	try { request = new XMLHttpRequest(); }
	catch (trymicrosoft)
	{
		try { request = new ActiveXObject("Msxml2.XMLHTTP"); }
		catch (othermicrosoft)
		{
			try { request = new ActiveXObject("Microsoft.XMLHTTP"); }
			catch (failed)
			{
				request = null;
			}
		}
	}
	if (request == null) alert("Error creating request object !");
	return request;
}

function send_request(url)
{
	if (AjaxReq == null) AjaxReq = createRequest();
	AjaxReq.open("GET", url, true);
	AjaxReq.onreadystatechange = update_page;
	AjaxReq.send(null);

	AjaxReqSend = 1;
}

function generate_random_str()
{
	var d = new Date();
	var str=d.getFullYear()+"."+(d.getMonth()+1)+"."+d.getDate()+"."+d.getHours()+"."+d.getMinutes()+"."+d.getSeconds();
	return str;
}

function update_state()
{
	if(AjaxReqSend == 0){
		send_request("load_balance_stats_chart_info.asp?t="+generate_random_str());
	}
	
}

function refresh_interface_page()
{
        update_state();
        setInterval(update_state, 1000);
}

function on_init_chart()
{
	refresh_interface_page();
}

function changeOnOffStateEvent(bt, wan_name)
{
	var patd = bt.src;
	var onoff_text	= document.getElementById(wan_name+"_onoff_text");
	if (patd.indexOf("wan_off") != -1){
		bt.src = "./graphics/icon/wan_on.png";
		bt.value = "on";

		onoff_text.innerHTML = "On";	
		onoff_text.style.color = "#229954";
	}else if (patd.indexOf("wan_on") != -1){
		bt.src = "./graphics/icon/wan_off.png";
		bt.value = "off";

		onoff_text.innerHTML = "Off";
		onoff_text.style.color = "#707070";
	}

	var onoff_change_val = wan_name + "_" + bt.value;
//	console.log("onoff_change_val", onoff_change_val);
	document.formLdStatsChart_demo.onoff_change.value = onoff_change_val;
	
	on_submit(bt);
}

function on_submit(obj)
{
	obj.isclick = 1;
	postTableEncrypt(document.formLdStatsChart_demo.postSecurityFlag, document.formLdStatsChart_demo);
	document.formLdStatsChart_demo.submit();

	return true;
}

</script>
</head>


<body onload="on_init_chart();">
<br>

<form action=/boaform/formLdStatsChart_demo method=POST name="formLdStatsChart_demo">
<table style="table-layout:fixed;">
<%rtk_load_balance_chart_header_info(); %>

<%rtk_load_balance_actual_wans(); %>

<div type="hidden">
	<input type="hidden" value="/load_balance_stats_chart.asp" name="submit-url">
	<input type="hidden" value="" name="onoff_change">
	<input type="hidden" name="postSecurityFlag" value="">
</div>

</table>
</form>
</body>

</html>

