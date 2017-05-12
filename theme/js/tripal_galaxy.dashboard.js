Drupal.behaviors.tripalGalaxyDashboard = {
  attach: function (context, settings) {
    
  
    /**
     * Renders the graphs for the Galaxy Admin Dashboard.
     * 
     * id: the element id into which the charts will be drawn.
     * fData: The data to be rendered in graphs.
     */
    function dashboard(id, fData){
      
      // Set aside 10 colors
      var c10 = d3.scale.category10();
      
      // Function to handle histogram.
      function barChart(fD){
          var hG={}, hGDim = {t: 60, r: 0, b: 30, l: 0};
          if (fD.length > 0) {
            hGDim.w = (900 * fD.length/10) - hGDim.l - hGDim.r;
          }
          else {
            hGDim.w = 900 - hGDim.l - hGDim.r;
          }
          hGDim.h = (300) - hGDim.t - hGDim.b;
                        
          // Create svg for histogram.
          var hGsvg = d3.select(id).append("svg")
              .attr("width", hGDim.w + hGDim.l + hGDim.r)
              .attr("height", hGDim.h + hGDim.t + hGDim.b).append("g")
              .attr("transform", "translate(" + hGDim.l + "," + hGDim.t + ")");
    
          // create function for x-axis mapping.
          var x = d3.scale.ordinal().rangeRoundBands([0, hGDim.w], 0.1)
                  .domain(fD.map(function(d) { return d.name;}));

          // Add x-axis to the histogram svg.
          var xAxis = d3.svg.axis()
            .scale(x)
            .orient("bottom");
          
          hGsvg.append("g")
            .attr("class", "x tripal-galaxy-usage-axis")
            .attr("transform", "translate(0," + hGDim.h + ")")
            .call(xAxis);
    
          // Create function for y-axis map.
          var y = d3.scale.linear().range([hGDim.h, 0])
                  .domain([0, d3.max(fD, function(d) { return d.count; })]);
    
          var yAxis = d3.svg.axis()
            .scale(y)
            .orient("left");
      
          hGsvg.append("g")
            .attr("class", "y tripal-galaxy-usage-axis")
            .call(yAxis);
          
          // Create bars for histogram to contain rectangles and labels.
          var bars = hGsvg.selectAll(".bar").data(fD.map(function(d) {return [d.name, d.count]})).enter()
                  .append("g").attr("class", "bar");
          
          // Create the rectangles.
          bars.append("rect")
              .attr("x", function(d) { return x(d[0]); })
              .attr("y", function(d) { return y(d[1]); })
              .attr("width", x.rangeBand())
              .attr("height", function(d) { return hGDim.h - y(d[1]); })
              .attr('fill', function(d, i) { return c10(i); })
              
          // Create the labels above the rectangles.
          bars.append("text").text(function(d){ return d3.format(",")(d[1])})
              .attr("x", function(d) { return x(d[0])+x.rangeBand()/2; })
              .attr("y", function(d) { return y(d[1])-5; })
              .attr("text-anchor", "middle");
          
      }
      
      // function to handle legend.
      function legend(lD){
          var leg = {};
              
          // create table for legend.
          var legend = d3.select(id).append("table").attr('class', 'tripal-galaxy-usage-legend');
          
          // create one row per segment.
          var tr = legend.append("tbody").selectAll("tr").data(lD).enter().append("tr");
              
          // create the first column for each segment.
          tr.append("td").append("svg").attr("width", '16').attr("height", '16').append("rect")
              .attr("width", '16').attr("height", '16')
              .attr("fill",function(d, i){ return c10(i); });
              
          // create the second column for each segment.
          tr.append("td").text(function(d){ return d.name;});
    
          return leg;
      }
      
     // Create the histogram and legends.
      var hG = barChart(fData), 
          leg= legend(fData);  
    }
    
    
    dashboard('#tripal-galaxy-usage-users-chart', top10users);
    dashboard('#tripal-galaxy-usage-workflows-chart', top10workflows);
  
  }
};