<%SendWebHeadStr(); %>
<SCRIPT>
function on_submit()
{
	document.forms[0].refresh.isclick = 1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
	return true;
}
</SCRIPT>
</head>

<body>
<style>
  .message {
     margin-left: 20px; /* Adjust this value to increase/decrease the space */
   }
</style>

<div class="intro_main ">
	<br>
	<h1 style='margin-left: 20px;'>
	<p class="intro_title">유효하지 않은 페이지 입니다. </p>
</div>

<form>

	<br>
	<h1 style='margin-left: 20px;'>
	<div class="btn_ctl">
		<input type="hidden" value="/admin/invalid_page.asp" name="submit-url">
		<input class="link_bg" type="submit" value="<% multilang(LANG_CLOSE); %>" name="close" onClick="return on_submit()">&nbsp;&nbsp;
		<input type="hidden" name="postSecurityFlag" value="">
	</div>
	</h1>
</form>
<br><br>
</body>

</html>
