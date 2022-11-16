<?php

require_once 'common.php';

list($qty, $desc) = get_query_params();

$stmt = "select 
    pd.classnum,
    count(pd.classnum) as qty
from point_data pd  
group by pd.classnum
";

$stmt .= "order by qty ";
if ($desc) $stmt .= "desc";
$stmt .= "\n";

$prepared_stmt = $_db->prepare($stmt);
$prepared_stmt->execute();

$ret_array = array();
foreach ($prepared_stmt->fetchAll() as $row) {
    $ret_array[] = array(
        "class" => classnum_to_class($row["classnum"]),
        "player_count" => $row["qty"],
    );
}

echo json_encode($ret_array);