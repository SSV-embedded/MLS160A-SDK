
	$(function() {

		// We use an inline data source in the example, usually data would
		// be fetched from a server

		var accx = [],
		    accy = [],
		    accz = [],
		    gyrx = [],
		    gyry = [],
		    gyrz = [];

		var datasets = {

            "ACCX": {
                label: "ACCX",
                data: []
            },
            "ACCY": {
                label: "ACCY",
                data: []
            },
            "ACCZ": {
                label: "ACCZ",
                data: []
            },
            "GYRX": {
                label: "GYRX",
                data: []
            },
            "GYRY": {
                label: "GYRY",
                data: []
            },
            "GYRZ": {
                label: "GYRZ",
                data: []
		}
	};

		var points_plot = 1000;
		var signal_sz = 10;

		var legend_container = document.getElementById("legend-container");

		var plot = $.plot("#placeholder", datasets, {
			legend: {
			    show: true,
			    pos: "ne",
			    margin: [20, 0]
			},
			yaxis: {
			    autoScaleMargin: 0.1,
				autoScale: 'loose'
			},
			xaxis: {
				show: true
			},
			lines: {
			    show: true
			},
			series: {
                shadowSize: 0	// Drawing is faster without shadows
			}
		});

		// hard-code color indices to prevent them from shifting as
		// countries are turned on/off

		var i = 0;
		$.each(datasets, function(key, val) {
			val.color = i;
			++i;
		});

		// insert checkboxes
		var choiceContainer = $("#choices");
		$.each(datasets, function(key, val) {
			choiceContainer.append("<input type='checkbox' class='choice' name='" + key +
				"' checked='checked' id='id" + key + "'></input>" +
				"<label for='id" + key + "'>"
				+ val.label + "</label>");
		});

		choiceContainer.find("input").click(plotAccordingToChoices);

		function plotAccordingToChoices() {

            var data = [];

            $("#choices").find("input:checked").each(function () {
                var key = $(this).attr("name")
                if (key && datasets[key]) {
                    data.push(datasets[key]);
                }
            });
		    return data;
	    }

		var socket = io();
        
        if (document.visibilityState == "visible") {

		socket.on('signals', function(msg){

            if (accx.length >= points_plot){
                accx = accx.slice(signal_sz);
                accy = accy.slice(signal_sz);
                accz = accz.slice(signal_sz);
                gyrx = gyrx.slice(signal_sz);
                gyry = gyry.slice(signal_sz);
                gyrz = gyrz.slice(signal_sz);

            }

		    for (var i= 0; i < signal_sz; i++) {

                accx.push(msg.ACCX[i]);
                accy.push(msg.ACCY[i]);
                accz.push(msg.ACCZ[i]);
                gyrx.push(msg.GYRX[i]);
                gyry.push(msg.GYRY[i]);
                gyrz.push(msg.GYRZ[i]);
			}

			//console.log(accx);


			var res_accx = [],
			    res_accy = [],
			    res_accz = [],
			    res_gyrx = [],
			    res_gyry = [],
			    res_gyrz = [];

			for (var i = 0; i < accx.length; i++) {
				res_accx.push([i, accx[i]])
				res_accy.push([i, accy[i]])
				res_accz.push([i, accz[i]])
				res_gyrx.push([i, gyrx[i]])
				res_gyry.push([i, gyry[i]])
				res_gyrz.push([i, gyrz[i]])
			}

			datasets.ACCX.data = res_accx;
            datasets.ACCY.data = res_accy;
            datasets.ACCZ.data = res_accz;
            datasets.GYRX.data = res_gyrx;
            datasets.GYRY.data = res_gyry;
            datasets.GYRZ.data = res_gyrz

            window.requestAnimationFrame(update);

        });
       }

        function update(){

			plot.setData(plotAccordingToChoices(datasets));
			plot.setupGrid(true);
			plot.draw();
        }

	});