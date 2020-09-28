function onButton()
{  /* Fonction pour le bouton ON */
    console.log('send led on');
    connection.send("{\"led\":1}");
}
function offButton()
{  /* Fonction pour le bouton OFF */
    console.log('send led off');
    connection.send("{\"led\":0}");
}

var connection = new WebSocket('ws://' + location.hostname + '/ws');
connection.onopen = function ()
{
    connection.send('{\"Connect\":\"' + new Date() + '\"}');
};

connection.onerror = function (error)
{
    console.log('WebSocket Error ', error);
};


connection.onmessage = function (event)
{
    try
    {
        var msg = JSON.parse(event.data);
    }
    catch (e)
    {
        console.error("Parsing error:", e);
        console.log(event.data);
    }
    var text = "";
    if (typeof msg.stamp != 'undefined')
    {
        var time = new Date(msg.stamp);
        var timeStr = time.toLocaleTimeString();

        text = "<b>donnée à " + timeStr + "</b><br>";
        text += "<b>température " + msg.temp + " °C</b><br>";
        text += "<b>hygrométrie " + msg.hydr + " %HR</b><br>";

        document.jauge.series[0].setData([msg.temp], true);
        document.jauge.series[1].setData([msg.hydr], true);

        if (text.length) {
            document.getElementById("display").innerHTML = text;
        }
    }
    console.log('Server: ', event.data);
};

var urlInput = location.href + 'temperature.csv';

function createChart() {
    Highcharts.chart('container', {
        chart: {
            type: 'spline'
        },
        title: {
            text: 'Relevé de température'
        },
        accessibility: {
            announceNewData: {
                enabled: true,
                minAnnounceInterval: 15000,
                announcementFormatter: function (allSeries, newSeries, newPoint) {
                    if (newPoint) {
                        return 'New point added. Value: ' + newPoint.y;
                    }
                    return false;
                }
            }
        },
        yAxis: [{
            min: -10,
            max: 40,
            labels: {
                format: '{value}°C'
            },
            title: {
                text: 'température'
            },
            opposite: true
        }, {
            gridLineWidth: 0,
            min: 0,
            max: 100,
            labels: {
                format: '{value} %HR',
            },
            title: {
                text: 'hygrométrie'
            }
        }],
        data: {
            csvURL: urlInput,
            enablePolling: true,
            dataRefreshRate: 2
        },
        series: [{
        }, {
            yAxis: 1
        }]
    });
}

document.jauge = new Highcharts.chart({
    chart: {
        renderTo: "containerVU",
        type: 'gauge',
        plotBorderWidth: 1,
        plotBackgroundColor: {
            linearGradient: { x1: 0, y1: 0, x2: 0, y2: 1 },
            stops: [
                [0, '#FFF4C6'],
                [0.3, '#FFFFFF'],
                [1, '#FFF4C6']
            ]
        },
        plotBackgroundImage: null,
        height: 200
    },

    title: {
        text: 'Température & Hygrométrie'
    },

    pane: [{
        startAngle: -45,
        endAngle: 45,
        background: null,
        center: ['25%', '145%'],
        size: 300
    }, {
        startAngle: -45,
        endAngle: 45,
        background: null,
        center: ['75%', '145%'],
        size: 300
    }],
    exporting: {
        enabled: false
    },
    credits: {
        enabled: false
    },
    tooltip: {
        enabled: false
    },

    yAxis: [{
        min: -10,
        max: 40,
        minorTickPosition: 'outside',
        tickPosition: 'outside',
        labels: {
            rotation: 'auto',
            distance: 10
        },
        plotBands: [{
            from: 30,
            to: 40,
            color: '#C02316',
            innerRadius: '100%',
            outerRadius: '105%'
        }, {
            from: 25,
            to: 30,
            color: '#EA4D40',
            innerRadius: '100%',
            outerRadius: '105%'
        }, {
            from: -10,
            to: 0,
            color: '#00FFFF',
            innerRadius: '100%',
            outerRadius: '105%'
        }, {
            from: 0,
            to: 10,
            color: '#0080FF',
            innerRadius: '100%',
            outerRadius: '105%'
        }],
        pane: 0,
        title: {
            text: 'VU<br/><span style="font-size:8px">température</span>',
            y: -40
        }
    }, {
        min: 0,
        max: 100,
        minorTickPosition: 'outside',
        tickPosition: 'outside',
        labels: {
            rotation: 'auto',
            distance: 20
        },
        plotBands: [{
            from: 0,
            to: 30,
            color: '#808000',
            innerRadius: '100%',
            outerRadius: '105%'
        }, {
            from: 60,
            to: 100,
            color: '#0000FF',
            innerRadius: '100%',
            outerRadius: '105%'
        }],
        pane: 1,
        title: {
            text: 'VU<br/><span style="font-size:8px">hygrométrie</span>',
            y: -40
        }
    }],

    plotOptions: {
        gauge: {
            dataLabels: {
                enabled: false
            },
            dial: {
                radius: '100%'
            }
        }
    },

    series: [{
        name: 'Channel A',
        data: [10],
        dataLabels: {
            formatter: function () {
                var temperature = this.y;
                return '<span style="color:#339">' + temperature + ' km/h</span>';
            }
        },
        yAxis: 0
    }, {
        name: 'Channel B',
        data: [10],
        yAxis: 1
    }]

},
);
    
// Create the chart
createChart();
