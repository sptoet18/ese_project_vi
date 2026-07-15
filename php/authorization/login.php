<?php
	require_once __DIR__ . '/../util.php';

    session_start();

    $db = new PDO(
        'mysql:host=127.0.1;dbname=__________',     // Add the database name
        '____',										// Add the username
		'____'										// Add the password
    );

	$db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);

	$username = $_POST['username'];
    $password = $_POST['password'];

	$_SESSION['username'] = $username;

	$userQuery = $db->prepare('
		select id, username, hashed_password
		from users
		where username = :username
	');
	$userQuery->execute(['username' => $username]);
	$user = $userQuery->fetch();

    if ($user) {
		isCorrectPassword($password, $user['hashed_password']);
		echo "<script>location.href = \"/php/authorization/member.php\"</script>";
	} else {
		echo "<script>location.href = \"/html/authorization/login.html\"</script>";
	}
?>
