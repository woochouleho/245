$(window).load(function(){
	var doc_h = $(document).height();
	var win_w = $(window).width();
	var path = window.location.pathname.split('/').pop();
	var n = 0;
	var url = "";
	var isBasic = false;
	var menu_button = 0;

	$(".gnb_close").on("click",function(){
		$(".gnb").stop().fadeOut(200);
		$(".util").stop().fadeOut(200);
	});

	$(".gnb_open").on("click",function(){
		$(".gnb").stop().fadeIn(200);
		$(".util").stop().fadeIn(200);
	});

	/*
	$('.custom_input input').iCheck({
		checkboxClass: 'icheckbox_minimal', 
		radioClass: 'iradio_minimal',
		increaseArea: '20%' 
	});
	*/

	/* zin77 block 20190418: temporary */
	/*
	$("body").iCheck({
		checkboxClass: 'icheckbox_minimal', 
		radioClass: 'iradio_minimal',
		increaseArea: '20%' 
	});
	*/

	$(".btn_setting li").on("click",function(){
		var a = $(this).index();
		$(".btn_setting li").removeClass("on");
		$(this).addClass("on");
		$(".gnb>ul").hide();
		if (a == 0){
			$(".basic_gnb").show();
		}else{
			$(".advanced_gnb").show();
		}
	});
	
	
	/* Neon, Refs #777 : show Root menu by loading page */
	for(n =0; n< $(".basic_gnb>li>div>ul>li>a").length; n++)
	{
		url = $(".basic_gnb>li>div>ul>li>a").get(n);
		if( path.indexOf( $(url).attr("href"))  != - 1) {
				isBasic = true;
				break;
		}
	}	
	
	/*
	 *	menu_button 0 - normal  basic
	 *	menu_button 1 - nomal   adv
	 *  menu_button 2 - mobile  basic
	 *  menu_button 3 - mobile  adv
	 */
	if( win_w <= 768)
		menu_button = isBasic ?  2 :  3;
	else
		menu_button = isBasic ?  0 :  1;

	
	if( isBasic == false) {
		$(".gnb>ul").hide();
		$(".advanced_gnb").show();
	}

	$(".btn_setting li").removeClass();
	$(".btn_setting li").each(function (index, item) {
		if(index == menu_button)
			$(item).addClass('on');
	});

	
	$(".gnb>ul").on("mouseenter",function(){
		$(".gnb>ul>li#on").attr('id','enter');
	});
	$(".gnb>ul").on("mouseleave",function(){
		$(".gnb>ul>li#enter").attr('id','on');
	});

	$(".speech_bubble_open").on("click",function(){
		$(".speech_bubble_txt").slideToggle();
		return false;
	});

	$(".popup_bg").on("click",function(){
		$(".popup_bg").stop().fadeOut(200);
		$(".popup").stop().fadeOut(250);
	});

	/* zin77 block 20190419: new mobile layout */
/*
	$(".popup_close").on("click",function(){
		$(".popup").fadeOut(200);
		$(".popup_cover").fadeOut(250);
	});
*/

	/*------ 웹 ------*/
	if (win_w >1120 ){

		$(".gnb>ul>li").on("mouseenter",function(){
			$(this).find(".s_gnb").show();
		});

		$(".gnb>ul>li").on("mouseleave",function(){
			$(this).find(".s_gnb").hide();
		});


	/*------ 태블릿 ------*/
	} else if (win_w <= 1120 && win_w >=768){

		$(".gnb>ul>li").on("mouseenter",function(){
			$(this).find(".s_gnb").show();
		});

		$(".gnb>ul>li").on("mouseleave",function(){
			$(this).find(".s_gnb").hide();
		});


	/*모바일*/
	} else if (win_w <= 768){

	var btn_gnb = $(".gnb>ul>li.mb_gnb>a");
	var sgnb_cont_all = btn_gnb.next();
	btn_gnb.on("click",function(){
		var sgnb_cont = $(this).next(":hidden");
		sgnb_cont_all.slideUp(200);
		sgnb_cont.slideDown(200);
		return false;
	});

	/* zin77 add 20190419: new mobile layout */
	$(".btn_setting li").on("click",function(){
		$(".gnb>ul>li.mb_gnb>a").unbind("click");
		var a = $(this).index();
		$(".btn_setting li").removeClass("on");
		$(this).addClass("on");
		$(".gnb>ul").hide();
		if (a == 0){
			$(".basic_gnb").show();
		}else{
			$(".advanced_gnb").show();
		}
		var btn_gnb = $(".gnb>ul>li.mb_gnb>a");
		var sgnb_cont_all = btn_gnb.next();
		btn_gnb.on("click",function(){
			var sgnb_cont = $(this).next(":hidden");
			sgnb_cont_all.slideUp(200);
			sgnb_cont.slideDown(200);
			return false;
		});
	});
	/* zin77 add 20190419: end */

	}

	$(window).resize(function(){
		var doc_h = $(document).height();
		var win_w = $(window).width();

		/*------ 웹 ------*/
		if (win_w >1120 ){
			$(".gnb>ul>li").unbind("mouseenter");
			$(".gnb>ul>li").unbind("mouseleave");
			$(".gnb>ul>li.mb_gnb>a").unbind("click");
			$(".gnb").show();

			$(".gnb>ul>li").on("mouseenter",function(){
				$(this).find(".s_gnb").show();
			});

			$(".gnb>ul>li").on("mouseleave",function(){
				$(this).find(".s_gnb").hide();
			});


		/*------ 태블릿 ------*/
		} else if (win_w <= 1120 && win_w >=768){
			$(".gnb>ul>li").unbind("mouseenter");
			$(".gnb>ul>li").unbind("mouseleave");
			$(".gnb>ul>li.mb_gnb>a").unbind("click");
			$(".gnb").show();

			$(".gnb>ul>li").on("mouseenter",function(){
				$(this).find(".s_gnb").show();
			});

			$(".gnb>ul>li").on("mouseleave",function(){
				$(this).find(".s_gnb").hide();
			});


		/*모바일*/
		} else if (win_w <= 768){
			$(".gnb>ul>li").unbind("mouseenter");
			$(".gnb>ul>li").unbind("mouseleave");
			$(".gnb>ul>li.mb_gnb>a").unbind("click");
			$(".gnb").hide();

		var btn_gnb = $(".gnb>ul>li.mb_gnb>a");
		var sgnb_cont_all = btn_gnb.next();
		btn_gnb.on("click",function(){
			var sgnb_cont = $(this).next(":hidden");
			sgnb_cont_all.slideUp(200);
			sgnb_cont.slideDown(200);
			return false;
		});

		}
	});


});

/* zin77 block 20190419: new mobile layout */
/*
function popup_open(name){
	var name = $(name);
	$(".popup_cover").fadeIn(200);
	name.fadeIn(250);
}
*/
