$(function() {
  var chart=$('#container').highcharts('StockChart', {
    series:DATA,
    title:
    {
     text:name,
     style:{
      color:'#111',
      fontWeight:'bold',
      fontSize:'12px'
     }
   },

   /******************* Настройки *********************/


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
  navigator:{
    height: 100
  },
  chart:{
    spacingBottom:1,
    spacingLeft:1,
    spacingRight:1,
    spacingTop:1
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
