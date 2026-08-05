<?php

// Note of all the class the user class will be commented the most

// add to the objects namespace
namespace objects;

require_once '../util.php';

// An enum to keep track of valid roles
enum Role: string {
    case admin = 'admin';
    case projectManager = 'project_member';
    case projectSupervisor = 'project_supervisor';
    case eseStudent = 'ese_student';
    case user = 'user';
}

// The user class
class User
{
    // All the variables (same as columns in the database)
    private int $id;
    private string $username;
    private string $firstname;
    private string $lastname;
    private Role $role;
    private \DateTimeImmutable $createdAt;
    private string $archived;

    //  Construct is private because we want to create in the database
    private function __construct() {}

    // This is how a user should be created
    public static function create(
        string $username,
        string $password,
        string $firstname,
        string $lastname,
        Role $role
    ): self {
        // Connect to the database
        $db = connect();

        // Hash the password for later
        $hashedPassword = password_hash($password, PASSWORD_DEFAULT);

        // Insert a user
        try {
            $query = '
                INSERT INTO user(
                                 username, 
                                 hashed_password, 
                                 firstname, 
                                 lastname, 
                                 role
                ) VALUES (
                          :username, 
                          :hashed_password, 
                          :firstname, 
                          :lastname, 
                          :role
                );
            ';
            $statement = $db->prepare($query);
            // Inser the information we want
            $statement->execute([
                'username' => $username,
                'hashed_password' => $hashedPassword,
                'firstname' => $firstname,
                'lastname' => $lastname,
                'role' => $role->value
            ]);

            // Get the ID of the user that was just created
            $id = $db->lastInsertId();
        } catch (\PDOException $e) {
            die("Creation failed: " . $e->getMessage());
        }

        // Get all the information about the user that was just created
        $user = self::get($id);

        if ($user === null) {
            throw new \RuntimeException("User {$id} was created but could not be found.");
        }

        return $user;
    }

    public static function get(int $id) : ?self {
        // Connect to the database
        $db = connect();

        try {
            // Get all the information about the user
            $query = '
                SELECT *
                FROM user
                WHERE id = :id
                    AND archived = false
                LIMIT 1
            ';
            $statement = $db->prepare($query);
            // Get the user based on the provided ID
            $statement->execute([
                'id' => $id
            ]);
            // Return the user as the correct class
            $row = $statement->fetch(\PDO::FETCH_ASSOC);
        } catch (\PDOException $e) {
            die("Get failed: " . $e->getMessage());
        }

        // If the user exists then return it, if not return null
        return $row ? self::fromRow($row) : null;
    }

    public function getId() : int {
        return $this->id;
    }

    public function getUsername() : string{
        return $this->username;
    }

    public function getFirstname() : string{
        return $this->firstname;
    }

    public function getLastname() : string {
        return $this->lastname;
    }

    public function getRole() : Role {
        return $this->role;
    }

    public function getCreatedAt() : \DateTimeImmutable {
        return $this->createdAt;
    }

    public function getArchived() : bool {
        return $this->archived;
    }

    private static function fromRow(array $row) : self {
        $user = new self();
        $user->id = (int) $row['id'];
        $user->username = (string) $row['username'];
        $user->firstname = (string) $row['firstname'];
        $user->lastname = (string) $row['lastname'];
        $user->role = Role::from($row['role']);
        $user->createdAt = new \DateTimeImmutable($row['created_at']);
        $user->archived = (bool) $row['archived'];

        return $user;
    }
}