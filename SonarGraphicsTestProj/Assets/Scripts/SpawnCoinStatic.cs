﻿using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class SpawnCoinStatic : MonoBehaviour {
	float x;
	float y; 
	float z;
	Vector3 pos;
    public AudioClip pickupSound;
    public AudioSource audioSource;
    public GameObject player;

    // Use this for initialization
    void Start () {
		player = GameObject.Find("FPSController 1");
        audioSource = player.GetComponent<AudioSource>();
    }
	
	// Update is called once per frame
	void Update () {
		transform.Rotate (0,0,50*Time.deltaTime);
	}


    void OnTriggerEnter(Collider Other)
    {
        if (Other.tag == "Player")
        {
            audioSource.clip = pickupSound;
            audioSource.Play();
            Destroy(gameObject);
        }
    }
}
