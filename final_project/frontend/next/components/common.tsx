import React from 'react'
import Head from 'next/head'
import { ToastContainer } from 'react-toastify';
import 'react-toastify/dist/ReactToastify.min.css';
import 'bootstrap/dist/css/bootstrap.min.css'

const Common = () => (
  <div>
    <Head>
      <link rel="icon" href="/favicon.ico" />
      <link rel="manifest" href="/manifest.json"></link>
    </Head>
    <ToastContainer />
  </div>
)

export default Common
