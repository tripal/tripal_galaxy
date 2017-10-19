// Using the closure to map jQuery to $. 
(function ($) {
  // Store our function as a property of Drupal.behaviors.
  Drupal.behaviors.tripal_galaxy = {
    attach: function (context, settings) {
      
      $(".tripal-galaxy-existing-sfile").change(function() {
        var value = $(this).val();
        var target_id = $(this).attr('target_id');
        $('#' + target_id).val(value);
      });
      $(".tripal-galaxy-site-wide-sfile").change(function() {
        var value = $(this).val();
        var target_id = $(this).attr('target_id');
        $('#' + target_id).val(value);
      });
      $(".tripal-galaxy-data-collection-sfile").change(function() {
        var value = $(this).val();
        var target_id = $(this).attr('target_id');
        $('#' + target_id).val(value);
      });

      /**
       * We use a global variable to keep track of which files have been
       * selected in the pflist form element. When the page first loads
       * we need to iniitalize the variable.
       */
      var tripal_galaxy_pfile_values = {};
      $(".tripal-galaxy-existing-pflist").each(function() {
        var value = $(this).val();
        var id =  $(this).attr('id');
        var table_id = $(this).attr('table_id');
        var target_id = $(this).attr('target_id');
        var which = $(this).attr('which_pair');
        
        // Intiailize the tripal_galaxy_pfile_values for this target.
        if (!(target_id in tripal_galaxy_pfile_values)) {
          tripal_galaxy_pfile_values[target_id] = {
              1 : {
                "id" : '',
                "values" : [],
              },
              2 : {
                "id" : '',
                "values" : [],
              },
          };
        }
        tripal_galaxy_pfile_values[target_id][which]["id"] = id;
      });
      
      /**
       * We use a global variable to keep track of which files have been
       * selected in the sflist form element. When the page first loads
       * we need to iniitalize the variable.
       */
      var tripal_galaxy_sfile_values = {};
      $(".tripal-galaxy-existing-sflist").each(function() {
        var value = $(this).val();
        var id =  $(this).attr('id');
        var table_id = $(this).attr('table_id');
        var target_id = $(this).attr('target_id');
        var which = $(this).attr('which_pair');
        
        // Intiailize the tripal_galaxy_sfile_values for this target.
        if (!(target_id in tripal_galaxy_sfile_values)) {
          tripal_galaxy_sfile_values[target_id] = {
            "id" : '',
            "values" : [],
          };
        }
        tripal_galaxy_sfile_values[target_id]["id"] = id;
      });
      
      /**
       * Rebuilds the table where the paired file list is displayed.
       * 
       * The values stored in the tripal_galaxy_pfile_values array
       * are displayed in a table for the user.  This function
       * iterates through that array to make the table rows.
       */
      function tripal_galaxy_rebuild_pflist_table(target_id, table_id) {
        // lastly, update the table with the names of the selected files
        var table_rows = '';
        var values1 = tripal_galaxy_pfile_values[target_id][1]["values"];
        var values2 = tripal_galaxy_pfile_values[target_id][2]["values"];
        var num_values1 = values1.length;
        var num_values2 = values2.length;
        var select1 = tripal_galaxy_pfile_values[target_id][1]["id"];
        var select2 = tripal_galaxy_pfile_values[target_id][2]["id"];
        var num_pairs = Math.max(num_values1, num_values2);
        var i = 0;

        // Get the values, disable the select box elements as appropriate,
        // and build each row.
        for (i = 0; i < num_pairs; i++) {
          var name1 = '';
          var name2 = '';
          if (num_values1 > i) {
            name1 = $('#' + select1 + " option[value='" + values1[i] + "']").text();
            $('#' + select1 + " option[value='" + values1[i] + "']").hide();
            $('#' + select2 + " option[value='" + values1[i] + "']").hide();
            $('#' + select1).val(0);
          }
          if (num_values2 > i) { 
            name2 = $('#' + select2 + " option[value='" + values2[i] + "']").text();
            $('#' + select1 + " option[value='" + values2[i] + "']").hide();
            $('#' + select2 + " option[value='" + values2[i] + "']").hide();
            $('#' + select2).val(0);
          }
          
          var trclass = 'odd';
          if (i % 2 == 0) {
            trclass = 'even';
          }
          
          table_rows += '<tr class="' + trclass + '">';
          table_rows +=   '<td>' + (i + 1) + '</td>';
          table_rows +=   '<td>' + name1 + '</td>';
          table_rows +=   '<td>' + name2 + '</td>';
          if (i == num_pairs - 1 && num_pairs > 0) {
            table_rows +=   '<td><span id="tripal-galaxy-existing-pflist-remove"></span></td>';
          }
          else {
            table_rows +=   '<td></td>';
          }
          table_rows += '</tr>';
        }
        
        // If we have no rows of paired files then add back in the empty row.
        if (num_pairs == 0) {
          table_rows += '<tr class="' + trclass + '">';
          table_rows +=   '<td colspan="4">There are no files.</td>';
          table_rows += '</tr>';
        }
        
        // Set the table rows.
        $('#' + table_id + ' > tbody').html(table_rows);
        
        // Now add the remove link. The remove link needs a click function
        // that will take the last row out of the array and clean up the table.
        link = $("<a>", {
          'class': 'tripal-galaxy-existing-pflist-remove',
          'href': 'javascript:void(0);',
          'text': ' Remove',
        });
        link.attr('target_id', target_id);
        link.attr('table_id', table_id);
        link.appendTo('#tripal-galaxy-existing-pflist-remove');
        
        // Add the click function to the link.
        link.click(function(){
          var target_id = $(this).attr('target_id');
          var table_id = $(this).attr('table_id');
          var values1 = tripal_galaxy_pfile_values[target_id][1]["values"];
          var values2 = tripal_galaxy_pfile_values[target_id][2]["values"];
          var num_values1 = values1.length;
          var num_values2 = values2.length;
          var select1 = tripal_galaxy_pfile_values[target_id][1]["id"];
          var select2 = tripal_galaxy_pfile_values[target_id][2]["id"];
          var num_pairs = Math.max(num_values1, num_values2);
          
          // Put the option back into both select boxes and pop the last 
          // element off of the tripal_galaxy_pfile_values array.
          if (num_pairs == num_values1) {
            $('#' + select1 + " option[value='" + values1[num_pairs - 1] + "']").show();
            $('#' + select2 + " option[value='" + values1[num_pairs - 1] + "']").show();
            tripal_galaxy_pfile_values[target_id][1]["values"].pop();
          }
          if (num_pairs == num_values2) {
            $('#' + select1 + " option[value='" + values2[num_pairs - 1] + "']").show();
            $('#' + select2 + " option[value='" + values2[num_pairs - 1] + "']").show();
            tripal_galaxy_pfile_values[target_id][2]["values"].pop();
          }

          tripal_galaxy_rebuild_pflist_table(target_id, table_id);
        });
        
        // Set the value array.
        var fids = '';
        if (values1) {
          fids += values1.join('|');
        }
        fids += ',';
        if (values2) {
          fids += values2.join('|');
        }
        $('#' + target_id).val(fids);
      }

      /**
       * Rebuilds the table where the single file list is displayed.
       * 
       * The values stored in the tripal_galaxy_sfile_values array
       * are displayed in a table for the user.  This function
       * iterates through that array to make the table rows.
       */
      function tripal_galaxy_rebuild_sflist_table(target_id, table_id) {
        // lastly, update the table with the names of the selected files
        var table_rows = '';
        var values = tripal_galaxy_sfile_values[target_id]["values"];
        var num_values = values.length;
        var select = tripal_galaxy_sfile_values[target_id]["id"];
        var i = 0;

        // Get the values, disable the select box elements as appropriate,
        // and build each row.
        for (i = 0; i < num_values; i++) {
          var name = '';
          if (num_values > i) {
            name = $('#' + select + " option[value='" + values[i] + "']").text();
            $('#' + select + " option[value='" + values[i] + "']").hide();
            $('#' + select).val(0);
          }
          
          var trclass = 'odd';
          if (i % 2 == 0) {
            trclass = 'even';
          }
          
          table_rows += '<tr class="' + trclass + '">';
          table_rows +=   '<td>' + (i + 1) + '</td>';
          table_rows +=   '<td>' + name + '</td>';
          if (i == num_values - 1 && num_values > 0) {
            table_rows +=   '<td><span id="tripal-galaxy-existing-sfile-remove"></span></td>';
          }
          else {
            table_rows +=   '<td></td>';
          }
          table_rows += '</tr>';
        }
        
        // If we have no rows of files then add back in the empty row.
        if (num_values == 0) {
          table_rows += '<tr class="' + trclass + '">';
          table_rows +=   '<td colspan="3">There are no files.</td>';
          table_rows += '</tr>';
        }
        
        // Set the table rows.
        $('#' + table_id + ' > tbody').html(table_rows);
        
        // Now add the remove link. The remove link needs a click function
        // that will take the last row out of the array and clean up the table.
        link = $("<a>", {
          'class': 'tripal-galaxy-existing-sfile-remove',
          'href': 'javascript:void(0);',
          'text': ' Remove',
        });
        link.attr('target_id', target_id);
        link.attr('table_id', table_id);
        link.appendTo('#tripal-galaxy-existing-sfile-remove');
        
        // Add the click function to the link.
        link.click(function(){
          var target_id = $(this).attr('target_id');
          var table_id = $(this).attr('table_id');
          var values = tripal_galaxy_sfile_values[target_id]["values"];
          var num_values = values.length;
          var select = tripal_galaxy_sfile_values[target_id]["id"];
          
            $('#' + select + " option[value='" + values[num_values - 1] + "']").show();
            tripal_galaxy_sfile_values[target_id]["values"].pop();
          

          tripal_galaxy_rebuild_sflist_table(target_id, table_id);
        });
        
        // Set the value array.
        var fids = '';
        if (values) {
          fids += values.join('|');
        }
        $('#' + target_id).val(fids);
      }
      /**
       * OnChange function for the paired file select boxes.
       * 
       * When the user selects a file in the select box it needs to add
       * the selected value to the tripal_galaxy_pfile_values array.
       */
      $(".tripal-galaxy-existing-pflist").change(function() {
        var value = $(this).val();
        var id =  $(this).attr('id');
        var table_id = $(this).attr('table_id');
        var target_id = $(this).attr('target_id');
        var which = $(this).attr('which_pair');
        
        // First find terms that should be added to our selected values list.
        if (value != 0) {  
          var num_selected = tripal_galaxy_pfile_values[target_id][which]["values"].length;
          var j = 0;
          var is_selected = false;
          for (j = 0; j < num_selected; j++) {
            if (tripal_galaxy_pfile_values[target_id][which]["values"][j] == value) {
              is_selected = true;
            }
          }
          if (!is_selected) {
            tripal_galaxy_pfile_values[target_id][which]["values"].push(value);
          }
        }
        
        // Rebuild the paired file table.
        tripal_galaxy_rebuild_pflist_table(target_id, table_id);
      });
      
      /**
       * OnChange function for the single file select boxes.
       * 
       * When the user selects a file in the select box it needs to add
       * the selected value to the tripal_galaxy_sfile_values array.
       */
      $(".tripal-galaxy-existing-sfile").change(function() {
        var value = $(this).val();
        var id =  $(this).attr('id');
        var table_id = $(this).attr('table_id');
        var target_id = $(this).attr('target_id');
        
        // First find terms that should be added to our selected values list.
        if (value != 0) {  
          var num_selected = tripal_galaxy_sfile_values[target_id]["values"].length;
          var j = 0;
          var is_selected = false;
          for (j = 0; j < num_selected; j++) {
            if (tripal_galaxy_sfile_values[target_id]["values"][j] == value) {
              is_selected = true;
            }
          }
          if (!is_selected) {
            tripal_galaxy_sfile_values[target_id]["values"].push(value);
          }
        }
        
        // Rebuild the paired file table.
        tripal_galaxy_rebuild_sflist_table(target_id, table_id);
      });
    }
  }
})(jQuery);