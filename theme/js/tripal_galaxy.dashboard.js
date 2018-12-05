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
          var chart_width = 600;
          var chart_height = 300;
          var top_margin = 30;
          var left_margin = 300;
          var right_margin = 30;
                        
          // Create SVG for histogram.
          var hGsvg = d3.select(id).append("svg")
              .attr("width", chart_width)
              .attr("height", chart_height)
              .append("g");
              
    
          // Create function for mapping of data to x and y axes.
          var x = d3.scale.linear().range([0, chart_width - (left_margin + right_margin)])
            .domain([0, d3.max(fD, function(d) { return d.count; })]);
          
          var y = d3.scale.ordinal().rangeRoundBands([0, chart_height - top_margin], 0.1)
            .domain(fD.map(function(d) { return d.name;}));
          
          // Create function for y-axis map.
          var yAxis = d3.svg.axis()
            .scale(y)
            .orient("left");
          
          var xAxis = d3.svg.axis()
            .scale(x)
            .ticks(4)
            .tickSubdivide(0);
          
          // Add axes to the histogram svg.  
          hGsvg.append("g")
            .attr("class", "x tripal-galaxy-usage-axis")
            .attr("transform", "translate(" + left_margin + ", 0)")
            .call(xAxis);
          
          hGsvg.append("g")
            .attr("class", "y tripal-galaxy-usage-axis")
            .attr("transform", "translate(" + left_margin + ", " + top_margin + ")")
            .call(yAxis)
          
          // Create bars for histogram to contain rectangles and labels.
          var bars = hGsvg.selectAll(".bar")
             .data(fD.map(function(d) {return [d.name, d.count]}))
             .enter()
             .append("g")
             .attr("class", "bar");

          // Create the rectangles.
          bars.append("rect")
              .attr("x", function(d) { return left_margin; })
              .attr("y", function(d) { return y(d[0])  + top_margin; })
              .attr("height", y.rangeBand())
              .attr("width", function(d) { return x(d[1]); })
              .attr('fill', function(d, i) { return c10(i); })
              
          // Create the quantity labels to the right of the rectangles.
          bars.append("text").text(function(d){ return d3.format(",")(d[1])})
              .attr("x", function(d) { return left_margin + x(d[1]) + 10; })
              .attr("y", function(d) { return y(d[0]) + (y.rangeBand()/2) + top_margin; })
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
      var hG = barChart(fData); 
          //leg= legend(fData);  
    }
    
    
    dashboard('#tripal-galaxy-usage-users-chart', top10users);
    dashboard('#tripal-galaxy-usage-workflows-chart', top10workflows);
  
  }
};