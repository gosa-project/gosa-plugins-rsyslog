<?php
   if ($remove_lock) {
     if (session::is_set('dfsManagment')) {
       $dfsManagment  = session::get('dfsManagment');
       $dfsManagment->remove_lock();
       del_lock($ui->dn);
       session::un_set('dfsManagment');
     }
   } else {
     # create dfsManagment object
     if (!session::is_set('dfsManagment') || 
        (isset($_GET['reset']) && $_GET['reset'] == 1)) {
       session::set('dfsManagment',new dfsManagment($config, $ui));
     }
     $dfsManagment  = session::get('dfsManagment');
     $output = $dfsManagment->execute();

     # do we have to reset
     if (isset($_GET['reset']) && $_GET['reset'] == 1) {
       del_lock ($ui->dn);
       session::un_set('dfsManagment');
     }
 
     # create page header
     if (session::is_set('objectinfo')) {
       $display = print_header(get_template_path('plugins/dfs/images/dfs.png'), 
                                _("Distributed File System Administration"),
                                "<img alt=\"\" class=\"center\" src=\"".
                                get_template_path('images/lists/locked.png').
                                "\"> ".session::get('objectinfo'));
     } else {
       $display = print_header(get_template_path('plugins/dfs/images/dfs.png'),
                                _("Distributed File System Administration"));
     }

     $display.= $output;
    
     # show page
     session::set('dfsManagment',$dfsManagment);
  }

?>