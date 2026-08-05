package main


//cgo CFLAGS: -I.
//cgo LDFLAGS: -L. -libc.so

//#include <stdio.h>
//#include <stdlib.h>
/*
//import  "libc.so"
*/
import "C"
import (
	"encoding/binary"
	"fmt"
	"unsafe"
)

type NProfile struct {
	Name  [20]byte
	Ipv4  uint32
	ID    int32
	Port  int32
}

func main() {
	// Call the C function to create the profile and get the raw pointer
	cProfile := C.create_profile()

	// Convert the C pointer to a Go pointer
	profileBytes := (*[unsafe.Sizeof(NProfile{})]byte)(unsafe.Pointer(cProfile))

	// Unmarshal the profile data from bytes to Go struct
	var profile NProfile
	err := unmarshalProfile(profileBytes[:], &profile)
	if err != nil {
		fmt.Println("Error unmarshaling profile:", err)
		return
	}

	// Print the Go struct fields
	fmt.Printf("Name: %s\n", profile.Name)
	fmt.Printf("IPv4: %d\n", profile.Ipv4)
	fmt.Printf("ID: %d\n", profile.ID)
	fmt.Printf("Port: %d\n", profile.Port)

	// Free the C memory
	C.free_profile(cProfile)
}

// UnmarshalProfile extracts the data from the byte slice into the NProfile struct
func unmarshalProfile(data []byte, profile *NProfile) error {
	// Ensure that the byte slice length matches the expected struct size
	if len(data) != int(unsafe.Sizeof(*profile)) {
		return fmt.Errorf("invalid byte slice length")
	}

	// Copy the name (first 20 bytes)
	copy(profile.Name[:], data[:20])

	// Extract the IPv4 field (next 4 bytes)
	profile.Ipv4 = binary.LittleEndian.Uint32(data[20:24])

	// Extract the ID field (next 4 bytes)
	profile.ID = int32(binary.LittleEndian.Uint32(data[24:28]))

	// Extract the Port field (next 4 bytes)
	profile.Port = int32(binary.LittleEndian.Uint32(data[28:32]))

	return nil
}

