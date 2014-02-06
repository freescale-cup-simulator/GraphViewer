$(function() {
        $('#container').highcharts('StockChart', {
        series:DATA,


/******************* Настройки *********************/

        legend:{
            enabled: true
        },

        navigator:{
            height:20
        },

        scrollbar:{
            height:10
        },

        chart: {
            backgroundColor:'rgba(255, 255, 255, 0.8)',
            
        },

        rangeSelector: {
            selected: 1
        },

        credits: {
            enabled: false
        },

        exporting: {
            enabled: false
        },
        xAxis:{
            labels:{
                formatter:function(){
                    return this.value;
                }
            }
        },
                                       tooltip: {
                                                  formatter: function() {
                                                      var s = '<b>'+ Highcharts.dateFormat('%L', this.x) +'</b>';

                                                      $.each(this.points, function(i, point) {
                                                          s += '<br/>'+point.series.name+': '+ point.y;
                                                      });

                                                      return s;
                                                  }
                                              },
                                       navigator:{xAxis:{
                                           labels:{
                                               formatter:function(){
                                                   return this.value;
                                               }
                                           }}
                                       },

        rangeSelector: {
            enabled:false
        }
    });
});
