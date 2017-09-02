// Using the closure to map jQuery to $. 
(function ($) {
  // Store our function as a property of Drupal.behaviors.
  Drupal.behaviors.tripal_galaxy = {
    attach: function (context, settings) {

      
    }
  }
  


})(jQuery);

/**
 * Used by the galaxy_pflist form element for setting paired files.
 * 
 * @param which
 *   Which pair (either 1 or 2)
 * @param tables
 *   The ID of the table that will will display the file list for the user.
 */
function tripal_galaxy_set_pfile_existing(which, table_id, select1, select2, target_id) {
  (function ($) {
    var values1 = $('#' + select1).val();
    var values2 = $('#' + select2).val(); 
    var num_values1 = 0;
    var num_values2 = 0;
    var i;
    var table_rows = '';
    var num_pairs = 0;
    var fids = '';
    
    if (values1) {
      num_values1 = values1.length;
    }
    if (values2) {
      num_values2 = values2.length;
    }
    num_pairs = Math.max(num_values1, num_values2);
    for (i = 0; i < num_pairs; i++) {
      var name1 = '';
      var name2 = '';
      if (num_values1 > i) {
        name1 = $('#' + select1 + " option[value='" + values1[i] + "']").text()
      }
      if (num_values2 > i) { 
        name2 = $('#' + select2 + " option[value='" + values2[i] + "']").text()
      }
      
      var trclass = 'odd';
      if (i % 2 == 0) {
        trclass = 'even';
      }
      
      table_rows += '<tr class="' + trclass + '">';
      table_rows +=   '<td>' + name1 + '</td>';
      table_rows +=   '<td>' + name2 + '</td>';
      table_rows += '</tr>';
    }
    
    // Set the table rows.
    $('#' + table_id + ' > tbody').html(table_rows);
    
    // Set the value array.
    if (values1) {
      fids += values1.join('|');
    }
    fids += ',';
    if (values2) {
      fids += values2.join('|');
    }
    $('#' + target_id).val(fids);
  })(jQuery);
}