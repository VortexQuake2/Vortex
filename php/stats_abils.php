<?php


require_once 'common.php';

list($qty, $desc) = get_query_params();

$stmt = "select 
    ab.\"index\" as id,
    sum(ab.level) as qty
from abilities ab  
group by ab.\"index\"
";

$stmt .= "order by qty ";
if ($desc) $stmt .= "desc";
$stmt .= "\n";

$stmt .= "limit :lim";

$prepared_stmt = $_db->prepare($stmt);
$prepared_stmt->execute(["lim" => $qty]);

$ret_array = array();
foreach ($prepared_stmt->fetchAll() as $row) {
    $ret_array[] = array(
        "ability" => abilnum_to_abil($row["id"]),
        "total_upgrades" => $row["qty"],
    );
}

echo json_encode($ret_array);