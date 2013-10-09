// globals for fixed tables
var querytab = null;
var exploretab = null;
var infotab = null;

// plugin code for reload
$.fn.dataTableExt.sErrMode = 'throw';
$.fn.dataTableExt.oApi.fnReloadAjax = function ( oSettings, sNewSource, fnCallback, bStandingRedraw )
{
    if ( typeof sNewSource != 'undefined' && sNewSource != null ) {
        oSettings.sAjaxSource = sNewSource;
    }
    this.oApi._fnProcessingDisplay( oSettings, true );
    var that = this;
    var iStart = oSettings._iDisplayStart;
    var aData = [];
  
    this.oApi._fnServerParams( oSettings, aData );
    oSettings.fnServerData( oSettings.sAjaxSource, aData, function(json) {
        /* Clear the old information from the table */
        that.oApi._fnClearTable( oSettings );
        /* Got the data - add it to the table */
        var aData =  (oSettings.sAjaxDataProp !== "") ?
            that.oApi._fnGetObjectDataFn( oSettings.sAjaxDataProp )( json ) : json;
        for ( var i=0 ; i<aData.length ; i++ ) {
            that.oApi._fnAddData( oSettings, aData[i] );
        }
        oSettings.aiDisplay = oSettings.aiDisplayMaster.slice();
        that.fnDraw();
        if ( typeof bStandingRedraw != 'undefined' && bStandingRedraw === true ) {
            oSettings._iDisplayStart = iStart;
            that.fnDraw( false );
        }
        that.oApi._fnProcessingDisplay( oSettings, false );
        /* Callback user function - for event handlers etc */
        if ( typeof fnCallback == 'function' && fnCallback != null ) {
            fnCallback( oSettings );
        }
    }, oSettings );
};

// click function for results
function resulthandler() {
    var item = $(this).attr('href');
    var key = item.slice(4);
    $('.result'+key).toggle();
    if ( $('.result'+key).is(':visible') ) {
		var save=$(this).html().replace(/^\+/,'-');
		$(this).html(save);
        oTable = $('#resulttab'+key).dataTable( {
			"bJQueryUI": true,
			"sPaginationType":"full_numbers",
			"sDom": '<"H"lfT>rt<"F"ip>',
            "oTableTools" : { "sSwfPath" : "/static/copy_csv_xls.swf" ,
			  "aButtons": ["copy", "csv", "print"]	
            },        
            "bProcessing": false,
            "bJQueryUI": true,
            "bRetrieve": true,
			"bAutoWidth":false,
            "aaSorting": [[4,"desc"]],
			 "aLengthMenu": [[10, 25, 50, 100, 500, -1], [10, 25, 50, 100, 500, "All"]],
            "sAjaxSource":"/_get_result?id="+key,
			"fnInitComplete": function() {
				this.fnAdjustColumnSizing(true);
			},
            "aoColumns": [
                { "sTitle":"Query Set" }, 
                { "sTitle":"Query"}, 
                { "sTitle":"Target Set"},
                { "sTitle":"Target"},
                { "sTitle":"Score" ,"sClass": "center", "bSearchable":false}

            ] //,
        } );
		oTable.fnReloadAjax();

    } else {
		var save=$(this).html().replace(/^-/,'+');
		$(this).html(save);
	}
}
	

function removehandler() {
	var item=$(this).attr('id');
	var key = item.slice(3);
	$.get($SCRIPT_ROOT + '/_remove', {id:key}, 
	function(data){
		// wait until it comes back to remove it
		$('#del'+key).parent().remove();
	});
}

// Adds a new result to the results listing
function resultlink(key,desc) {
	var container = $('<div/>', {
		'class':'resultlisting ui-state-highlight ui-corner-all'
	}).insertAfter('.topmarker');
    var newlink = $('<a/>', {
        'class':'result-list',
        href: '#res'+key,
        html: "+ "+desc+"&nbsp;"});
    newlink.click(resulthandler);
    newlink.appendTo(container)
	$('<span/>', {
        'class':'resstatus',
		'id':key }).appendTo(newlink);
	removal=$('<a/>', {
		'style': 'float:right;',
		'class':'remove',
		'id':'del'+key,
		'href':'#',
		'html':' remove' 
	});
	removal.click(removehandler);
	removal.appendTo(container);
	
    $('<div/>', {
        'style':'display:none;',
        'class':'result'+key}).appendTo(container);
	$('<table>', {
            id:'resulttab'+key,
            cellpadding:'0',
            cellspacing:'0',
            border:'0',
            'class': 'display'
	}).appendTo('.result'+key);
    $('<br/>').appendTo(container);
	//$(container).after('tddopmarker');
}

// Adds a new result to the results listing
function statusupdate() {
	// get list of results jquery iter all matching spans
	var mark=0;
	$('.resstatus').each(function() {
			var key=$(this).attr('id');
			mark++;
			if ($(this).text().substring(0,8)!='complete') {
			$.get($SCRIPT_ROOT + '/_res_status', {id:key}, 
				function(data) {
					$('#'+key).empty();
					$('#'+key).append(data);
				});
			} else {
				if ($('#'+key).parent().parent().hasClass('ui-state-highlight')) {
					$('#'+key).parent().parent().removeClass('ui-state-highlight');
					$('#'+key).parent().parent().addClass('ui-state-default');
				}
				mark--;
			}
		});
	//});
	$('.newhash').each(function() {
		var key=$(this).attr('id');
		mark++;
		if ($(this).text().substring(0,9)=='completed') {
			$('#'+key).remove();
			mark--;
			// refresh tables. and delete. delete not yet
		} else {
			$.get($SCRIPT_ROOT + '/_hashset_status', {id:key}, 
			function(data) {
				$('#'+key).empty();
				$('#'+key).append(data);
				if (data.substring(0,9)=='completed') {
					querytab.fnReloadAjax();
					exploretab.fnReloadAjax();
				}
			});
		}
	});
	if (mark<=0){
		// note, do we want to leave this up with a dismiss button instead?
		$('#newres').empty();
		$('#latest').hide();
	}
}


// RUNNING CODE BEGINS HERE
// document ready function, provides handlers
$(function() {
	$('#tabs').tabs();
    // code for plus/minus icons
	// adding handlers for existing results
	$('.result-list').click(resulthandler); 
	$('.remove').click(removehandler); 
		// handler for the set listing
	$('.set-list').click( function () {
		var item= $(this).attr('href');
		var key = item.slice(4);
		$('.set'+key).toggle();
		if ( $('.set'+key).is(':visible') ) {
		  $.getJSON($SCRIPT_ROOT + '/_hashset', {id:key}, 
			function(data) {
			var items = [];
			$.each(data, function(key, val) {
				items.push('<li id="' + key + '">' + val + '</li>');
			 });
			 $('.hashset-list'+key).remove()
			 $('<ul/>', {
					'class': 'hashset-list'+key,
					html: items.join('')
			 }).appendTo('.set'+key);
		});
		};
	});
	$("#samplebox").click(function() {
		$("#sample").toggle();
	});
	$("#confidencebox").click(function() {
		$("#confidence").toggle();
	});
	// defined function for our comparison button
	// defined function for our comparison button
		$('#compare').click( function() {
			//var query = $("#newres").empty();
			var none = true;
			var sample = 0;
			if ($("#samplebox:checked").length) {
				sample= $("#sample").val();
			}
			var threshold = 1;
			if ($("#confidencebox:checked").length) {
				threshold=$("#confidence").val();
			}
			var set1TT = TableTools.fnGetInstance( 'set1' );
			var querylist = set1TT.fnGetSelectedData();
			set1TT.fnSelectNone();
			var selectct = querylist.length;
			if (selectct == 1) {
				none = false;
				// change to CompareAll function
				$.each(querylist,function(key,val) {
					var set1=val[0];
					var title1=val[1];
					$.get($SCRIPT_ROOT+ '/_compare_all', {set1:set1, thr:threshold},
					function(data) {
						$.each(data ,function(key,val) {
							resultlink(val,title1+" all hashes");
						});
					});
				});
			} else if (selectct>1) {
				// dumb counting allowing us to sneak past foreach iterations
				var outer=0;
				var inner=0;
				$.each(querylist,function(key,val) {
					var set1=val[0];
					var title1=val[1];
					outer++;
					inner=0;
					$.each(querylist,function(key2,val2) {
							inner++;
							none = false;
							var set2=val2[0];
							var title2=val2[1];	
							if (set1!=set2 && inner > outer) {
							    $.get($SCRIPT_ROOT+ '/_compare', {set1:set1,set2:set2, samp:sample, thr:threshold},
								function(data) {
									$.each(data ,function(key,val) {
										resultlink(val,title1+" -- "+title2);
									});
								});
							}
					});    
				});    
			}
			if (none) {
				$("#compareerror").show();
				$("#compareerror").html("Choose at least one set to compare");
				$("#compareerror").fadeOut(3000);

			} else {
				$("#set1 DTTT_selected").each(function() {
					$(this).removeClass('DTTT_selected');
				});
				$("#targetset DTTT_selected").each(function() {
					$(this).removeClass('DTTT_selected');
				});
				$("#tabs").tabs({selected:3});
				$("input:checked").removeAttr('checked');
				$("#sample").hide();
				$("#confidence").hide();
			}
		});
		$('#createhash').click( function() {
				var none = true;
				var tohash= [];
				var filename = $("#hashsetname").val();
				$("#filetable .DTTT_selected").each(function() {
						var title = $(this).text();
						none=false;
						tohash.push(title);
					});
				if (none) {
					$("#hasherror").show();
					$("#hasherror").html("Choose at least one file");
					$("#hasherror").fadeOut(3000);
				} else if ($("#hashsetname").val().length < 2) {
					$("#hasherror").show();
					$("#hasherror").html("Choose a name");
					$("#hasherror").fadeOut(3000);
				} else {
					$('#latest').show();
					var requesthash=tohash.join('|');
					$.post($SCRIPT_ROOT+ '/_hash', {hashsetname:filename,sourcefiles:requesthash},
						function(data) {
							$.each(data,function(res,id) {
							$('<span/>', {
							'class':'newhash',
							'id':id,
							text:'processing '+filename }).appendTo('.newhashes');
						});
					});
				$("#filetable .DTTT_selected").each(function() {
                    $(this).removeClass('DTTT_selected');
                });
				$("#hashsetname").val('');
				$("#tabs").tabs({selected:1});
            }
    });

	var filetab =$('#filetable').dataTable( {
			"bJQueryUI": true,
			"sPaginationType":"full_numbers",
"sDom": '<"H"lfTi>rt<"F"ip>',
            "oTableTools" : { "sSwfPath" : "/static/copy_csv_xls_pdf.swf",
			  "aButtons": ["select_all", "select_none" ],
			  "sRowSelect": "multi" //,
            },        
			"iDisplayLength":25,
            "bProcessing": true,
            "bRetrieve": true,
			"aLengthMenu": [[25, 50, 100, 500, -1], [25, 50, 100, 500, "All"]],
            "sAjaxSource":"/_files",
            "aoColumns": [
                { "sTitle":"Number", "sWidth":"2em", "bVisible":false },//,
                { "sTitle":"Filename" }
			]
	});
	querytab=$('#set1').dataTable( {
			"bJQueryUI": true,
			"bPaginate":false,
			"bLengthChange":true,
"sDom": '<"fg-toolbar ui-widget-header ui-corner-tl ui-corner-tr ui-helper-clearfix">t<"fg-toolbar ui-widget-header ui-corner-bl ui-corner-br ui-helper-clearfix">T<"clear">',
            "oTableTools" : { "sSwfPath" : "/static/copy_csv_xls_pdf.swf",
			  "aButtons": [ ],
			  "sRowSelect": "multi" //,
            },        
            "bProcessing": false,
            "bRetrieve": true,
            "sAjaxSource":"/_get_listing",
            "aaSorting": [[1,"asc"]],
            "aoColumns": [
                { "sTitle":"ID", "sWidth":"4em","bVisible":false},//,
                { "sTitle":"Available Hashsets"},
                { "sTitle":"Size", "sWidth":"3em","sClass":"Rightside" },
                { "sTitle":"Items", "sWidth": "3em","sClass":"Rightside" },
			]
	});
// todo -- make this move to explore tab
//$('#set1 tbody td.Clickable').live('click',function(){
    //var aPos = querytab.fnGetPosition(this);
    //var aData = querytab.fnGetData(aPos[0]);
    //alert(aData[0]);
	// update table with listing
//});
	exploretab=$('#setex').dataTable( {
			"bJQueryUI": true,
			"bPaginate":false,
			"bLengthChange":true,
"sDom": '<"fg-toolbar ui-widget-header ui-corner-tl ui-corner-tr ui-helper-clearfix">t<"fg-toolbar ui-widget-header ui-corner-bl ui-corner-br ui-helper-clearfix">T<"clear">',
            "oTableTools" : { "sSwfPath" : "/static/copy_csv_xls_pdf.swf",
			  "aButtons": [ ],
			  "sRowSelect": "single" //,
            },        
            "bProcessing": false,
            "bRetrieve": true,
            //"bServerSide": true,
            "sAjaxSource":"/_get_listing",
            "aaSorting": [[1,"asc"]],
            "aoColumns": [
                { "sTitle":"ID", "sWidth":"4em","bVisible":false},//,
                { "sTitle":"Available Hashsets"},
                { "sTitle":"Size", "sWidth":"3em","sClass":"Rightside" },
                { "sTitle":"Items", "sWidth": "3em","sClass":"Rightside" },
                { "sTitle":"", "sDefaultContent":'>>', "sWidth":"0.75em","sClass":"Clickable", "bSortable":false}
			]
	});
$('#setex tbody tr').live('click',function(){
    var aPos = exploretab.fnGetPosition(this);
	//alert(aPos);
    var aData = exploretab.fnGetData(aPos);
	//alert(aData);
	var key= aData[0];
	infotab = $('#infotable').dataTable( {
			"bJQueryUI": true,
			"bLengthChange":true,
            "bProcessing": true,
            "bDestroy": true,
			"fnInitComplete": function() {
				this.fnAdjustColumnSizing(true);
			},
            "sAjaxSource":"/_hashset?id="+key,
            "aoColumns": [
                { "sTitle":"ID", "bVisible":false},//,
                { "sTitle":"File Name"},
                { "sTitle":"Size", "sClass":"Rightside" }
			]
	});
	// update table with listing
	infotab.fnReloadAjax();
});
	// disabling submit buttons to be javascript buttons
	$("form").submit(function() { return false });
	statusupdate();
    setInterval(statusupdate,5000);
});
