function html_code_to_script_str(str)
{
	if(str == "" || str == null)
		return str;
	else
	{
		return str.replace(/&quot;/gi,"\"").replace(/&#39;/gi,"\'")
				.replace(/&#92;/gi,"\\").replace(/&#38;/gi,"&")
				.replace(/&lt;/gi,"<").replace(/&gt;/gi,">")
				.replace(/&#40;/gi,"(").replace(/&#41;/gi,")")
				.replace(/&#35;/gi,"#");
	}
}

function html_code_to_script_arr(arr, len)
{
	for(var i=0; i<len; i++)
	{
		html_code_to_script_str(arr[i]);
	}
}

function is_dfs_ch_num(ch_id)
{
	var ch_val= $("#"+ch_id).val();
	var ch_num_str= new String();
	for(var i=0; $.isNumeric(ch_val[i]); i++)
		ch_num_str+= ch_val[i];
	if(ch_num_str == "0")
		return "auto";
	return ((48< Number(ch_num_str)) && (Number(ch_num_str)< 149));
}

function chbox_off_confirm(id_name ,confirm_msg)
{
	var id_check = document.getElementById(id_name).checked;
	if ( id_check == false )
	{
		if ( confirm(confirm_msg) == false )
			$("#"+id_name).prop("checked",true);
	}
}
