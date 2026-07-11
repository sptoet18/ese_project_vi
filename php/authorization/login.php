<?php
    session_start();

    $username = $_POST['username'];
    $password = $_POST['password'];

    // check if the username and passwords work
    if ($username === "seantoet" && $password === "seantoet") {
        $_SESSION['username'] = $username;
        echo "<script>location.href = \"/php/authorization/member.php\"</script>";
    } else {
        echo "<script>location.href = \"/html/authorization/login.html\"</script>";
    }
?>
