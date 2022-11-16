<?php
require_once 'common.php';

list($qty, $desc) = get_query_params();

$stmt = "select 
    ud.title, 
    ud.playername, 
    pd.exp,
    pd.level,
    pd.classnum
from point_data pd
inner join userdata as ud on ud.char_idx = pd.char_idx  
";

$stmt .= "order by pd.exp ";
if ($desc) $stmt .= "desc";
$stmt .= "\n";

$stmt .= "limit :lim";

$prepared_stmt = $_db->prepare($stmt);
$prepared_stmt->execute(['lim' => $qty]);

$ret_array = array();
foreach ($prepared_stmt->fetchAll() as $row) {
    $ret_array[] = array(
        "title" => $row["title"],
        "playername" => $row["playername"],
        "exp" => $row["exp"],
        "level" => $row["level"],
        "class" => classnum_to_class($row["classnum"])
    );
}

echo json_encode($ret_array);